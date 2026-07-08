from datetime import date, timedelta
import logging
import random
import traceback
from fastapi import FastAPI
from pydantic import BaseModel

logging.basicConfig(level=logging.INFO)

app = FastAPI(title="otelapi-py", version="1.0.1")
logger = logging.getLogger("otelapi-py")

SUMMARIES = [
    "Freezing",
    "Bracing",
    "Chilly",
    "Cool",
    "Mild",
    "Warm",
    "Balmy",
    "Hot",
    "Sweltering",
    "Scorching",
]


class WeatherForecast(BaseModel):
    date: date
    temperatureC: int
    temperatureF: int
    summary: str


class CustomTestException(Exception):
    """Custom exception used by test endpoint."""


@app.get("/weatherforecast", response_model=list[WeatherForecast])
def get_weather_forecast() -> list[WeatherForecast]:
    logger.info("Python weather forecast requested")
    forecasts: list[WeatherForecast] = []

    for index in range(1, 6):
        temperature_c = random.randint(-20, 55)
        forecasts.append(
            WeatherForecast(
                date=date.today() + timedelta(days=index),
                temperatureC=temperature_c,
                temperatureF=32 + int(temperature_c / 0.5556),
                summary=random.choice(SUMMARIES),
            )
        )

    return forecasts


@app.get("/throw-custom-exception")
def throw_custom_exception() -> None:
    logger.warning("Python custom exception test endpoint called")
    raise CustomTestException("This is a Python custom exception for testing.")


@app.get("/throw-and-catch-exception")
def throw_and_catch_exception() -> dict[str, object]:
    try:
        raise CustomTestException(
            "This exception is thrown and handled in Python test endpoint."
        )
    except Exception as ex:
        print(f"[Handled Exception] {type(ex).__name__}: {ex}")
        print(traceback.format_exc())
        logger.warning("Python handled exception test endpoint caught an exception")

    return {
        "message": "Exception was thrown, caught, and printed to console.",
        "handled": True,
    }
