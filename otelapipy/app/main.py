from datetime import date, timedelta
import logging
import random
from fastapi import FastAPI
from pydantic import BaseModel

logging.basicConfig(level=logging.INFO)

app = FastAPI(title="otelapi-py", version="1.0.0")
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
