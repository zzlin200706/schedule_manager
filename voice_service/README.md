# MySchedule Voice AI Service

This directory contains the local FastAPI service boundary for voice-assisted task creation.

Current stage:

- `GET /health` reports service status and whether `DEEPSEEK_API_KEY` is configured.
- `POST /stt` validates a WAV upload, writes it to a random temporary file, transcribes it with local `faster-whisper`, trims the transcript, and deletes the temporary file.
- `POST /parse-task` calls DeepSeek's OpenAI-compatible Chat Completions API and validates the returned task JSON.
- The Whisper model is loaded once when the service starts. If `faster-whisper` or the model is unavailable, `/health` reports `whisper_loaded: false` and `/stt` returns `WHISPER_UNAVAILABLE`.
- If `DEEPSEEK_API_KEY` is missing, the service still starts and `/parse-task` returns `DEEPSEEK_API_KEY_MISSING`.
- No API key is returned by any endpoint.

## Setup

```bash
cd voice_service
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
```

Edit `.env` and set `DEEPSEEK_API_KEY` before using `/parse-task`.

Default faster-whisper settings are tuned for local CPU inference:

```env
WHISPER_MODEL=small
WHISPER_DEVICE=cpu
WHISPER_COMPUTE_TYPE=int8
WHISPER_LANGUAGE=zh
WHISPER_BEAM_SIZE=3
WHISPER_CPU_THREADS=4
```

## Run

```bash
uvicorn app:app --host 127.0.0.1 --port 8765
```

Keep the service bound to `127.0.0.1`. Do not expose it on `0.0.0.0` for normal local use.
When the Qt voice feature is enabled, the GUI first checks this local endpoint and starts the service from `.venv/bin/python` if it is not already running.

## Tests

The unit tests use fake Whisper and DeepSeek clients. They do not download model files or call the real DeepSeek API:

```bash
python3 -m unittest test_stt_service.py test_deepseek_task_parser.py
```
