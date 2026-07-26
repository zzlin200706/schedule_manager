"""DeepSeek task parser service boundary."""

# 文件职责：
# 本文件封装 DeepSeek API 调用。它把语音识别后的自然语言文本组织成
# chat/completions 请求，并校验模型返回的任务 JSON 字段。
#
# 主要功能定位：
# - 功能：解析语音任务信息
# - 调用 DeepSeek API 解析任务名称、开始时间、提醒时间、优先级和分类

from __future__ import annotations

import json
from typing import Any, Callable, Optional

import httpx
from pydantic import ValidationError

from config import Settings
from schemas import ParsedTaskResponse, ParseTaskRequest
from stt_service import VoiceServiceError


HTTP_CLIENT_FACTORY = Callable[[], Any]
MAX_MODEL_CONTENT_CHARS = 4096


# DeepSeek 任务字段解析器，只负责远程请求和响应校验。
class DeepSeekTaskParser:
    def __init__(
        self,
        settings: Settings,
        client_factory: Optional[HTTP_CLIENT_FACTORY] = None,
    ) -> None:
        self._settings = settings
        self._client_factory = client_factory

    async def parse(self, request: ParseTaskRequest) -> ParsedTaskResponse:
        # ==================== 功能：解析语音任务信息 ====================
        # 步骤 1：确认 API Key 已配置，并拒绝空文本。
        if not self._settings.deepseek_key_configured:
            raise VoiceServiceError(
                503,
                "DEEPSEEK_API_KEY_MISSING",
                "DEEPSEEK_API_KEY is not configured for the voice service.",
            )

        if request.text.strip() == "":
            raise VoiceServiceError(
                400,
                "EMPTY_TEXT",
                "Task text cannot be empty.",
            )

        payload = self._build_request_payload(request)
        try:
            # 步骤 2：调用 DeepSeek chat/completions，请求模型只返回 JSON。
            async with self._make_client() as client:
                response = await client.post(
                    self._chat_completions_url(),
                    headers=self._headers(),
                    json=payload,
                )
        except (ValueError, RuntimeError) as error:
            raise VoiceServiceError(
                503,
                "DEEPSEEK_NETWORK_ERROR",
                f"Failed to initialize DeepSeek HTTP client: {error}",
            ) from error
        except httpx.TimeoutException as error:
            raise VoiceServiceError(
                504,
                "DEEPSEEK_TIMEOUT",
                "DeepSeek API request timed out.",
            ) from error
        except httpx.HTTPError as error:
            raise VoiceServiceError(
                503,
                "DEEPSEEK_NETWORK_ERROR",
                "Failed to connect to DeepSeek API.",
            ) from error

        # 步骤 3：处理 HTTP 状态、解析响应 JSON，并提取 message.content。
        if response.status_code < 200 or response.status_code >= 300:
            self._raise_for_remote_status(response.status_code)

        try:
            response_json = response.json()
        except ValueError as error:
            raise VoiceServiceError(
                502,
                "DEEPSEEK_INVALID_RESPONSE",
                "DeepSeek API returned a non-JSON response.",
            ) from error

        content = self._extract_message_content(response_json)
        if len(content) > MAX_MODEL_CONTENT_CHARS:
            raise VoiceServiceError(
                502,
                "DEEPSEEK_RESPONSE_TOO_LARGE",
                "DeepSeek API returned an oversized task parser response.",
            )

        try:
            # 步骤 4：模型内容必须是 JSON 对象，再交给 Pydantic 校验字段类型。
            parsed_json = json.loads(content)
        except json.JSONDecodeError as error:
            raise VoiceServiceError(
                502,
                "DEEPSEEK_INVALID_JSON",
                "DeepSeek API did not return valid task JSON.",
            ) from error

        if not isinstance(parsed_json, dict):
            raise VoiceServiceError(
                502,
                "DEEPSEEK_INVALID_JSON",
                "DeepSeek API task JSON must be an object.",
            )

        try:
            return _validate_parsed_task_response(parsed_json)
        except ValidationError as error:
            raise VoiceServiceError(
                502,
                "DEEPSEEK_INVALID_RESPONSE",
                "DeepSeek API returned task fields with invalid types.",
            ) from error

    def _build_request_payload(self, request: ParseTaskRequest) -> dict[str, Any]:
        # 请求中包含当前时间和默认值，方便模型解析“今天/明天”等相对时间。
        return {
            "model": self._settings.deepseek_model,
            "messages": [
                {
                    "role": "system",
                    "content": self._system_prompt(request),
                },
                {
                    "role": "user",
                    "content": request.text.strip(),
                },
            ],
            "temperature": 0.1,
            "max_tokens": 500,
            "thinking": {
                "type": "disabled",
            },
            "response_format": {
                "type": "json_object",
            },
        }

    def _system_prompt(self, request: ParseTaskRequest) -> str:
        # 系统提示词约束模型只输出固定 JSON 结构，缺失字段显式放入 missing_fields。
        return f"""你是日程管理软件中的任务信息提取器。

你的唯一职责是把用户的自然语言转换为 JSON。
只输出一个合法 JSON 对象，不要输出 Markdown、代码块、解释、问候或其他文本。

当前本地时间：{request.current_datetime}
时区：{request.timezone}

允许的 priority：
- high
- medium
- low

允许的 category：
- study
- entertainment
- life

用户未说明优先级时返回 null，C++ 会使用默认值：{request.default_priority}
用户未说明分类时返回 null，C++ 会使用默认值：{request.default_category}

必须输出以下 JSON 结构：

{{
  "title": "任务名称",
  "start_time": "YYYY-MM-DD HH:MM 或 null",
  "reminder_time": "YYYY-MM-DD HH:MM 或 null",
  "priority": "high、medium、low 或 null",
  "category": "study、entertainment、life 或 null",
  "missing_fields": []
}}

规则：

1. 根据当前本地时间解析今天、明天、后天、本周、下周等相对时间。
2. “下午三点”转换为 15:00。
3. “晚上十二点”需要结合自然语言语境正确处理日期边界。
4. 如果无法确定任务名称，title 返回空字符串，并把 "title" 放入 missing_fields。
5. 如果无法确定开始时间，start_time 返回 null，并把 "start_time" 放入 missing_fields。
6. 用户未说明优先级时，priority 返回 null，不得擅自猜测。
7. 用户未说明分类时，category 返回 null，不得擅自猜测。
8. 用户未说明提醒时间时，reminder_time 返回 null。
9. 不得生成任务 ID、用户名、文件路径、命令或内部状态。
10. 不得根据常识虚构用户没有表达的日期、时间或任务内容。
11. 所有日期时间必须使用 YYYY-MM-DD HH:MM。
12. 只输出 JSON。"""

    def _make_client(self) -> Any:
        if self._client_factory is not None:
            return self._client_factory()
        return httpx.AsyncClient(timeout=self._settings.request_timeout_seconds)

    def _chat_completions_url(self) -> str:
        return self._settings.deepseek_base_url.rstrip("/") + "/chat/completions"

    def _headers(self) -> dict[str, str]:
        # API Key 只从环境配置读取并放入请求头，注释和日志中不输出真实 Key。
        return {
            "Authorization": "Bearer " + self._settings.deepseek_api_key,
            "Content-Type": "application/json",
        }

    def _extract_message_content(self, response_json: Any) -> str:
        if not isinstance(response_json, dict):
            raise VoiceServiceError(
                502,
                "DEEPSEEK_INVALID_RESPONSE",
                "DeepSeek API response must be a JSON object.",
            )

        choices = response_json.get("choices")
        if not isinstance(choices, list) or not choices:
            raise VoiceServiceError(
                502,
                "DEEPSEEK_EMPTY_RESPONSE",
                "DeepSeek API returned no choices.",
            )

        first_choice = choices[0]
        if not isinstance(first_choice, dict):
            raise VoiceServiceError(
                502,
                "DEEPSEEK_INVALID_RESPONSE",
                "DeepSeek API returned an invalid choice.",
            )

        message = first_choice.get("message")
        if not isinstance(message, dict):
            raise VoiceServiceError(
                502,
                "DEEPSEEK_INVALID_RESPONSE",
                "DeepSeek API returned an invalid message.",
            )

        content = message.get("content")
        if not isinstance(content, str) or content.strip() == "":
            raise VoiceServiceError(
                502,
                "DEEPSEEK_EMPTY_RESPONSE",
                "DeepSeek API returned empty task JSON.",
            )
        return content.strip()

    def _raise_for_remote_status(self, status_code: int) -> None:
        if status_code == 400:
            raise VoiceServiceError(
                400,
                "DEEPSEEK_REQUEST_INVALID",
                "DeepSeek API rejected the task parser request.",
            )
        if status_code in {401, 403}:
            raise VoiceServiceError(
                502,
                "DEEPSEEK_AUTH_FAILED",
                "DeepSeek API authentication failed.",
            )
        if status_code == 402:
            raise VoiceServiceError(
                502,
                "DEEPSEEK_BILLING_ERROR",
                "DeepSeek API account cannot complete the request.",
            )
        if status_code == 429:
            raise VoiceServiceError(
                503,
                "DEEPSEEK_RATE_LIMITED",
                "DeepSeek API rate limit was reached.",
            )
        if status_code >= 500:
            raise VoiceServiceError(
                502,
                "DEEPSEEK_SERVER_ERROR",
                "DeepSeek API returned a server error.",
            )
        raise VoiceServiceError(
            502,
            "DEEPSEEK_REQUEST_FAILED",
            "DeepSeek API request failed.",
        )


def _validate_parsed_task_response(data: dict[str, Any]) -> ParsedTaskResponse:
    # 兼容 Pydantic v1/v2 的校验入口，保持服务返回结构一致。
    validator = getattr(ParsedTaskResponse, "model_validate", None)
    if validator is not None:
        return validator(data)
    return ParsedTaskResponse.parse_obj(data)
