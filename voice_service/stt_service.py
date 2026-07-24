"""Speech-to-text service backed by local faster-whisper."""

from __future__ import annotations

import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

from config import Settings


MAX_AUDIO_BYTES = 25 * 1024 * 1024
MIN_WAV_BYTES = 44
MAX_TRANSCRIPT_CHARS = 2000
ALLOWED_CONTENT_TYPES = {
    "audio/wav",
    "audio/wave",
    "audio/x-wav",
    "application/octet-stream",
}


@dataclass
class VoiceServiceError(Exception):
    status_code: int
    code: str
    message: str


@dataclass
class SttResult:
    text: str
    language: str
    duration_ms: int


class SttService:
    def __init__(self, settings: Settings, load_model: bool = True) -> None:
        self._settings = settings
        self._model: Any = None
        self._whisper_loaded = False
        self._load_error = ""
        if load_model:
            self._load_model()

    @property
    def whisper_loaded(self) -> bool:
        return self._whisper_loaded

    @property
    def load_error(self) -> str:
        return self._load_error

    async def transcribe(self, audio: Any) -> SttResult:
        if not self._whisper_loaded or self._model is None:
            raise VoiceServiceError(
                503,
                "WHISPER_UNAVAILABLE",
                "faster-whisper is not available or the model failed to load.",
            )

        contents = await self._validate_audio_upload(audio)
        temp_path = self._write_temp_wav(contents)
        start_time = time.monotonic()
        try:
            segments, info = self._model.transcribe(
                str(temp_path),
                language=self._settings.whisper_language,
                beam_size=self._settings.whisper_beam_size,
            )
            text = self._collect_transcript(segments)
            language = getattr(info, "language", None) or self._settings.whisper_language
        except VoiceServiceError:
            raise
        except Exception as error:
            raise VoiceServiceError(
                500,
                "STT_FAILED",
                f"Whisper speech recognition failed: {error}",
            ) from error
        finally:
            temp_path.unlink(missing_ok=True)

        if not text:
            raise VoiceServiceError(
                422,
                "EMPTY_TRANSCRIPT",
                "Whisper did not return any transcript text.",
            )
        if len(text) > MAX_TRANSCRIPT_CHARS:
            raise VoiceServiceError(
                413,
                "TRANSCRIPT_TOO_LONG",
                "Whisper transcript exceeds the configured text length limit.",
            )

        return SttResult(
            text=text,
            language=str(language),
            duration_ms=max(int((time.monotonic() - start_time) * 1000), 0),
        )

    def _load_model(self) -> None:
        try:
            from faster_whisper import WhisperModel
        except ImportError as error:
            self._load_error = "faster-whisper is not installed."
            self._whisper_loaded = False
            self._model = None
            return

        try:
            self._model = WhisperModel(
                self._settings.whisper_model,
                device=self._settings.whisper_device,
                compute_type=self._settings.whisper_compute_type,
                cpu_threads=self._settings.whisper_cpu_threads,
            )
        except Exception as error:
            self._load_error = str(error)
            self._whisper_loaded = False
            self._model = None
            return

        self._load_error = ""
        self._whisper_loaded = True

    async def _validate_audio_upload(self, audio: Any) -> bytes:
        filename = (audio.filename or "").lower()
        if not filename.endswith(".wav"):
            raise VoiceServiceError(
                415,
                "UNSUPPORTED_AUDIO_FORMAT",
                "Only WAV audio uploads are accepted.",
            )

        content_type = (audio.content_type or "").lower()
        if content_type and content_type not in ALLOWED_CONTENT_TYPES:
            raise VoiceServiceError(
                415,
                "UNSUPPORTED_AUDIO_FORMAT",
                "Only WAV audio uploads are accepted.",
            )

        contents = await audio.read(MAX_AUDIO_BYTES + 1)
        if len(contents) > MAX_AUDIO_BYTES:
            raise VoiceServiceError(
                413,
                "AUDIO_TOO_LARGE",
                "Audio upload exceeds the configured size limit.",
            )
        if len(contents) <= MIN_WAV_BYTES:
            raise VoiceServiceError(
                400,
                "EMPTY_AUDIO",
                "Audio upload is empty or too small to contain WAV data.",
            )
        return contents

    def _write_temp_wav(self, contents: bytes) -> Path:
        handle = tempfile.NamedTemporaryFile(
            prefix="myschedule_voice_stt_",
            suffix=".wav",
            delete=False,
        )
        temp_path = Path(handle.name)
        try:
            with handle:
                handle.write(contents)
                handle.flush()
        except Exception:
            temp_path.unlink(missing_ok=True)
            raise
        return temp_path

    def _collect_transcript(self, segments: Iterable[Any]) -> str:
        parts = []
        for segment in segments:
            text = getattr(segment, "text", "")
            if text:
                parts.append(str(text))
        return "".join(parts).strip()
