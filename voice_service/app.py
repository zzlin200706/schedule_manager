"""Local FastAPI service for MySchedule voice features."""

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
    return HealthResponse(
        status="ok",
        whisper_loaded=stt_service.whisper_loaded,
        deepseek_key_configured=settings.deepseek_key_configured,
        stt_available=stt_service.whisper_loaded,
        parse_task_available=settings.deepseek_key_configured,
    )


@app.post("/stt", response_model=SttResponse)
async def stt(audio: UploadFile = File(...)) -> SttResponse:
    result = await stt_service.transcribe(audio)
    return SttResponse(
        text=result.text,
        language=result.language,
        duration_ms=result.duration_ms,
    )


@app.post("/parse-task", response_model=ParsedTaskResponse)
async def parse_task(request: ParseTaskRequest) -> ParsedTaskResponse:
    return await task_parser.parse(request)
