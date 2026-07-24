"""Configuration for the local voice AI service."""

from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path


def _load_dotenv_file() -> None:
    try:
        from dotenv import load_dotenv
    except ImportError:
        return

    env_path = Path(__file__).resolve().parent / ".env"
    load_dotenv(env_path)


def _get_int(name: str, default: int, minimum: int, maximum: int) -> int:
    raw_value = os.getenv(name)
    if raw_value is None or raw_value.strip() == "":
        return default
    try:
        value = int(raw_value)
    except ValueError:
        return default
    return min(max(value, minimum), maximum)


def _get_str(name: str, default: str) -> str:
    value = os.getenv(name)
    if value is None:
        return default
    value = value.strip()
    return value if value else default


@dataclass(frozen=True)
class Settings:
    deepseek_api_key: str
    deepseek_base_url: str
    deepseek_model: str
    whisper_model: str
    whisper_device: str
    whisper_compute_type: str
    whisper_language: str
    whisper_beam_size: int
    whisper_cpu_threads: int
    voice_service_host: str
    voice_service_port: int
    request_timeout_seconds: int

    @property
    def deepseek_key_configured(self) -> bool:
        return bool(self.deepseek_api_key)


def load_settings() -> Settings:
    _load_dotenv_file()
    return Settings(
        deepseek_api_key=os.getenv("DEEPSEEK_API_KEY", "").strip(),
        deepseek_base_url=_get_str("DEEPSEEK_BASE_URL", "https://api.deepseek.com"),
        deepseek_model=_get_str("DEEPSEEK_MODEL", "deepseek-v4-flash"),
        whisper_model=_get_str("WHISPER_MODEL", "small"),
        whisper_device=_get_str("WHISPER_DEVICE", "cpu"),
        whisper_compute_type=_get_str("WHISPER_COMPUTE_TYPE", "int8"),
        whisper_language=_get_str("WHISPER_LANGUAGE", "zh"),
        whisper_beam_size=_get_int("WHISPER_BEAM_SIZE", 3, 1, 20),
        whisper_cpu_threads=_get_int("WHISPER_CPU_THREADS", 4, 1, 64),
        voice_service_host=_get_str("VOICE_SERVICE_HOST", "127.0.0.1"),
        voice_service_port=_get_int("VOICE_SERVICE_PORT", 8765, 1, 65535),
        request_timeout_seconds=_get_int("REQUEST_TIMEOUT_SECONDS", 60, 1, 300),
    )
