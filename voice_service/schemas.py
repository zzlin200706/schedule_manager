"""Pydantic schemas for the local voice AI service."""

from __future__ import annotations

from typing import List, Optional

from pydantic import BaseModel, Field, StrictStr


class ErrorResponse(BaseModel):
    code: str
    message: str


class HealthResponse(BaseModel):
    status: str
    whisper_loaded: bool
    deepseek_key_configured: bool
    stt_available: bool
    parse_task_available: bool


class SttResponse(BaseModel):
    text: str
    language: str
    duration_ms: int = Field(ge=0)


class ParseTaskRequest(BaseModel):
    text: str = Field(min_length=1, max_length=2000)
    current_datetime: str = Field(min_length=1, max_length=32)
    timezone: str = Field(min_length=1, max_length=64)
    default_priority: str = Field(default="medium", max_length=16)
    default_category: str = Field(default="life", max_length=32)


class ParsedTaskResponse(BaseModel):
    title: StrictStr
    start_time: Optional[StrictStr]
    reminder_time: Optional[StrictStr]
    priority: Optional[StrictStr]
    category: Optional[StrictStr]
    missing_fields: List[StrictStr]
