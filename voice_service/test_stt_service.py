"""Unit tests for the STT service without loading a real Whisper model."""

from __future__ import annotations

import asyncio
from pathlib import Path
from types import SimpleNamespace
import unittest

from config import Settings
from stt_service import (
    MAX_TRANSCRIPT_CHARS,
    MIN_WAV_BYTES,
    SttService,
    VoiceServiceError,
)


class FakeUpload:
    def __init__(
        self,
        contents: bytes,
        filename: str = "audio.wav",
        content_type: str = "audio/wav",
    ) -> None:
        self.filename = filename
        self.content_type = content_type
        self._contents = contents

    async def read(self, size: int = -1) -> bytes:
        del size
        return self._contents


class FakeSegment:
    def __init__(self, text: str) -> None:
        self.text = text


class FakeModel:
    def __init__(self, transcript: str) -> None:
        self.transcript = transcript
        self.audio_path = ""
        self.beam_size = 0
        self.calls = 0

    def transcribe(self, audio_path: str, language: str, beam_size: int):
        self.audio_path = audio_path
        self.beam_size = beam_size
        self.calls += 1
        return [FakeSegment(self.transcript)], SimpleNamespace(language=language)


def make_settings() -> Settings:
    return Settings(
        deepseek_api_key="",
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


def make_wav_payload(extra: bytes = b"voice") -> bytes:
    return b"RIFF" + (b"0" * (MIN_WAV_BYTES - 4)) + extra


class SttServiceTest(unittest.TestCase):
    def make_loaded_service(self, model: FakeModel) -> SttService:
        service = SttService(make_settings(), load_model=False)
        service._model = model
        service._whisper_loaded = True
        service._load_error = ""
        return service

    def test_transcribes_wav_and_removes_temp_file(self) -> None:
        model = FakeModel("  明天下午三点写作业  ")
        service = self.make_loaded_service(model)

        result = asyncio.run(service.transcribe(FakeUpload(make_wav_payload())))

        self.assertEqual(result.text, "明天下午三点写作业")
        self.assertEqual(result.language, "zh")
        self.assertEqual(model.calls, 1)
        self.assertEqual(model.beam_size, 3)
        self.assertFalse(Path(model.audio_path).exists())

    def test_missing_whisper_model_is_clear_error(self) -> None:
        service = SttService(make_settings(), load_model=False)
        service._model = None
        service._whisper_loaded = False

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(service.transcribe(FakeUpload(make_wav_payload())))

        self.assertEqual(raised.exception.code, "WHISPER_UNAVAILABLE")

    def test_rejects_non_wav_upload(self) -> None:
        service = self.make_loaded_service(FakeModel("text"))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(
                service.transcribe(
                    FakeUpload(make_wav_payload(), filename="audio.mp3", content_type="audio/mpeg")
                )
            )

        self.assertEqual(raised.exception.code, "UNSUPPORTED_AUDIO_FORMAT")

    def test_rejects_empty_audio(self) -> None:
        service = self.make_loaded_service(FakeModel("text"))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(service.transcribe(FakeUpload(b"RIFF")))

        self.assertEqual(raised.exception.code, "EMPTY_AUDIO")

    def test_rejects_empty_transcript(self) -> None:
        service = self.make_loaded_service(FakeModel("   "))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(service.transcribe(FakeUpload(make_wav_payload())))

        self.assertEqual(raised.exception.code, "EMPTY_TRANSCRIPT")

    def test_rejects_too_long_transcript(self) -> None:
        service = self.make_loaded_service(FakeModel("x" * (MAX_TRANSCRIPT_CHARS + 1)))

        with self.assertRaises(VoiceServiceError) as raised:
            asyncio.run(service.transcribe(FakeUpload(make_wav_payload())))

        self.assertEqual(raised.exception.code, "TRANSCRIPT_TOO_LONG")


if __name__ == "__main__":
    unittest.main()
