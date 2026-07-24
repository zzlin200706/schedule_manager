"""Unit tests for DeepSeek task parsing without calling the real API."""

from __future__ import annotations

import asyncio
import json
from typing import Any, Optional
import unittest

import httpx

from config import Settings
from deepseek_task_parser import DeepSeekTaskParser
from schemas import ParseTaskRequest
from stt_service import VoiceServiceError


class FakeResponse:
    def __init__(self, status_code: int, body: Any) -> None:
        self.status_code = status_code
        self._body = body

    def json(self) -> Any:
        if isinstance(self._body, Exception):
            raise self._body
        return self._body


class FakeHttpClient:
    def __init__(
        self,
        response: Optional[FakeResponse] = None,
        error: Optional[Exception] = None,
    ) -> None:
        self.response = response
        self.error = error
        self.calls = 0
        self.last_url = ""
        self.last_headers: dict[str, str] = {}
        self.last_json: dict[str, Any] = {}

    async def __aenter__(self) -> "FakeHttpClient":
        return self

    async def __aexit__(self, exc_type: Any, exc: Any, traceback: Any) -> None:
        del exc_type, exc, traceback

    async def post(
        self,
        url: str,
        headers: dict[str, str],
        json: dict[str, Any],
    ) -> FakeResponse:
        self.calls += 1
        self.last_url = url
        self.last_headers = headers
        self.last_json = json
        if self.error is not None:
            raise self.error
        assert self.response is not None
        return self.response


def make_settings(api_key: str = "test_key") -> Settings:
    return Settings(
        deepseek_api_key=api_key,
        deepseek_base_url="https://api.deepseek.com",
        deepseek_model="deepseek-v4-flash",
        whisper_model="small",
        whisper_device="cpu",
        whisper_compute_type="int8",
        whisper_language="zh",
        whisper_beam_size=3,
        whisper_cpu_threads=4,
        voice_service_host="127.0.0.1",
        voice_service_port=8765,
        request_timeout_seconds=60,
    )


def make_request(text: str = "明天下午三点写数据结构作业，优先级高，分类学习") -> ParseTaskRequest:
    return ParseTaskRequest(
        text=text,
        current_datetime="2026-07-23 14:30:00",
        timezone="Asia/Shanghai",
        default_priority="medium",
        default_category="life",
    )


def make_chat_response(content: str, status_code: int = 200) -> FakeResponse:
    return FakeResponse(
        status_code,
        {
            "choices": [
                {
                    "message": {
                        "content": content,
                    },
                },
            ],
        },
    )


class DeepSeekTaskParserTest(unittest.TestCase):
    def make_parser(
        self,
        response: Optional[FakeResponse] = None,
        error: Optional[Exception] = None,
        api_key: str = "test_key",
    ) -> tuple[DeepSeekTaskParser, FakeHttpClient]:
        client = FakeHttpClient(response=response, error=error)
        parser = DeepSeekTaskParser(make_settings(api_key), client_factory=lambda: client)
        return parser, client

    def test_success_returns_parsed_task_response(self) -> None:
        content = json.dumps(
            {
                "title": "写数据结构作业",
                "start_time": "2026-07-24 15:00",
                "reminder_time": None,
                "priority": "high",
                "category": "study",
                "missing_fields": [],
            },
            ensure_ascii=False,
        )
        parser, client = self.make_parser(make_chat_response(content))

        result = asyncio.run(parser.parse(make_request()))

        self.assertEqual(result.title, "写数据结构作业")
        self.assertEqual(result.start_time, "2026-07-24 15:00")
        self.assertEqual(result.priority, "high")
        self.assertEqual(result.category, "study")
        self.assertEqual(client.calls, 1)
        self.assertEqual(client.last_url, "https://api.deepseek.com/chat/completions")
        self.assertEqual(client.last_headers["Authorization"], "Bearer test_key")
        self.assertEqual(client.last_json["model"], "deepseek-v4-flash")
        self.assertEqual(client.last_json["response_format"], {"type": "json_object"})
        self.assertIn("当前本地时间：2026-07-23 14:30:00", client.last_json["messages"][0]["content"])

    def test_missing_api_key_is_clear_error_without_http_call(self) -> None:
        parser, client = self.make_parser(make_chat_response("{}"), api_key="")

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(parser.parse(make_request()))

        self.assertEqual(raised.exception.code, "DEEPSEEK_API_KEY_MISSING")
        self.assertEqual(client.calls, 0)

    def test_empty_text_is_rejected(self) -> None:
        parser, client = self.make_parser(make_chat_response("{}"))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(parser.parse(make_request("   ")))

        self.assertEqual(raised.exception.code, "EMPTY_TEXT")
        self.assertEqual(client.calls, 0)

    def test_invalid_model_json_is_rejected(self) -> None:
        parser, _ = self.make_parser(make_chat_response("not json"))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(parser.parse(make_request()))

        self.assertEqual(raised.exception.code, "DEEPSEEK_INVALID_JSON")

    def test_invalid_response_field_types_are_rejected(self) -> None:
        content = json.dumps(
            {
                "title": 123,
                "start_time": "2026-07-24 15:00",
                "reminder_time": None,
                "priority": "high",
                "category": "study",
                "missing_fields": [],
            },
        )
        parser, _ = self.make_parser(make_chat_response(content))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(parser.parse(make_request()))

        self.assertEqual(raised.exception.code, "DEEPSEEK_INVALID_RESPONSE")

    def test_empty_choices_are_rejected(self) -> None:
        parser, _ = self.make_parser(FakeResponse(200, {"choices": []}))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(parser.parse(make_request()))

        self.assertEqual(raised.exception.code, "DEEPSEEK_EMPTY_RESPONSE")

    def test_non_json_api_response_is_rejected(self) -> None:
        parser, _ = self.make_parser(FakeResponse(200, ValueError("not json")))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(parser.parse(make_request()))

        self.assertEqual(raised.exception.code, "DEEPSEEK_INVALID_RESPONSE")

    def test_timeout_is_mapped_to_user_safe_error(self) -> None:
        parser, _ = self.make_parser(error=httpx.TimeoutException("timeout"))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(parser.parse(make_request()))

        self.assertEqual(raised.exception.code, "DEEPSEEK_TIMEOUT")

    def test_http_errors_are_mapped(self) -> None:
        cases = {
            400: "DEEPSEEK_REQUEST_INVALID",
            401: "DEEPSEEK_AUTH_FAILED",
            402: "DEEPSEEK_BILLING_ERROR",
            429: "DEEPSEEK_RATE_LIMITED",
            500: "DEEPSEEK_SERVER_ERROR",
        }

        for status_code, expected_code in cases.items():
            with self.subTest(status_code=status_code):
                parser, _ = self.make_parser(FakeResponse(status_code, {"error": "redacted"}))

                with self.assertRaises(VoiceServiceError) as raised:
                    asyncio.run(parser.parse(make_request()))

                self.assertEqual(raised.exception.code, expected_code)


if __name__ == "__main__":
    unittest.main()
