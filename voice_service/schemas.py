"""Pydantic schemas for the local voice AI service."""

# 文件职责：
# 本文件定义 FastAPI 请求和响应模型，确保 Qt GUI 与 Python 语音服务
# 使用稳定的 JSON 字段名称。

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
    # Qt 发送识别文本、当前时间、时区和默认任务字段，用于 DeepSeek 解析相对时间。
    text: str = Field(min_length=1, max_length=2000)
    current_datetime: str = Field(min_length=1, max_length=32)
    timezone: str = Field(min_length=1, max_length=64)
    default_priority: str = Field(default="medium", max_length=16)
    default_category: str = Field(default="life", max_length=32)


class ParsedTaskResponse(BaseModel):
    # DeepSeek 解析后的结构化任务字段；缺失字段由 missing_fields 告知 Qt 表单补全。
    title: StrictStr
    start_time: Optional[StrictStr]
    reminder_time: Optional[StrictStr]
    priority: Optional[StrictStr]
    category: Optional[StrictStr]
    missing_fields: List[StrictStr]
