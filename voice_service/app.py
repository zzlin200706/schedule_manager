"""Local FastAPI service for MySchedule voice features."""

# 文件职责：
# 本文件是本地语音服务入口，Qt GUI 会请求这里的 /health、/stt 和 /parse-task。
# 其中 /stt 负责语音转文字，/parse-task 负责把自然语言任务解析成结构化字段。
#
# 主要功能定位：
# - 功能：语音转文字
# - 功能：解析语音任务信息

from __future__ import annotations

from fastapi import FastAPI, File, Request, UploadFile
from fastapi.responses import JSONResponse

from config import load_settings
from deepseek_task_parser import DeepSeekTaskParser
from schemas import (
    ErrorResponse,
    HealthResponse,
    ParsedTaskResponse,
    ParseTaskRequest,
    SttResponse,
)
from stt_service import SttService, VoiceServiceError


settings = load_settings()
# 服务启动时加载配置、初始化 Whisper STT 服务和 DeepSeek 任务解析器。
stt_service = SttService(settings)
task_parser = DeepSeekTaskParser(settings)

app = FastAPI(
    title="MySchedule Voice AI Service",
    version="0.1.0",
    description="Local service boundary for faster-whisper STT and DeepSeek task parsing.",
)


@app.exception_handler(VoiceServiceError)
async def voice_service_error_handler(
    request: Request,
    error: VoiceServiceError,
) -> JSONResponse:
    del request
    return JSONResponse(
        status_code=error.status_code,
        content=ErrorResponse(code=error.code, message=error.message).dict(),
    )


@app.get("/health", response_model=HealthResponse)
async def health() -> HealthResponse:
    # Qt 开始录音前先调用健康检查，确认 STT 模型和 DeepSeek Key 是否可用。
    return HealthResponse(
        status="ok",
        whisper_loaded=stt_service.whisper_loaded,
        deepseek_key_configured=settings.deepseek_key_configured,
        stt_available=stt_service.whisper_loaded,
        parse_task_available=settings.deepseek_key_configured,
    )


@app.post("/stt", response_model=SttResponse)
async def stt(audio: UploadFile = File(...)) -> SttResponse:
    # ==================== 功能：语音转文字 ====================
    # 接收 Qt 上传的 WAV 文件，调用 SttService 返回识别文本。
    result = await stt_service.transcribe(audio)
    return SttResponse(
        text=result.text,
        language=result.language,
        duration_ms=result.duration_ms,
    )


@app.post("/parse-task", response_model=ParsedTaskResponse)
async def parse_task(request: ParseTaskRequest) -> ParsedTaskResponse:
    # ==================== 功能：解析语音任务信息 ====================
    # 接收已确认的识别文本，调用 DeepSeekTaskParser 输出任务字段 JSON。
    return await task_parser.parse(request)
