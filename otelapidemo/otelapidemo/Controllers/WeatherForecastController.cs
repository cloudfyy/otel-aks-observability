using Microsoft.AspNetCore.Mvc;
using otelapidemo.Exceptions;

namespace otelapidemo.Controllers
{
    [ApiController]
    [Route("[controller]")]
    public class WeatherForecastController : ControllerBase
    {
        private static readonly string[] Summaries = new[]
        {
            "Freezing", "Bracing", "Chilly", "Cool", "Mild", "Warm", "Balmy", "Hot", "Sweltering", "Scorching"
        };

        private readonly ILogger<WeatherForecastController> _logger;

        public WeatherForecastController(ILogger<WeatherForecastController> logger)
        {
            _logger = logger;
        }

        [HttpGet(Name = "GetWeatherForecast")]
        public IEnumerable<WeatherForecast> Get()
        {
            _logger.LogInformation(".NET weather forecast requested");

            return Enumerable.Range(1, 5).Select(index => new WeatherForecast
            {
                Date = DateOnly.FromDateTime(DateTime.Now.AddDays(index)),
                TemperatureC = Random.Shared.Next(-20, 55),
                Summary = Summaries[Random.Shared.Next(Summaries.Length)]
            })
            .ToArray();
        }

        [HttpGet("throw-custom-exception")]
        public IActionResult ThrowCustomException()
        {
            _logger.LogWarning(".NET custom exception test endpoint called");
            throw new CustomTestException("This is a .NET custom exception for testing.");
        }

        [HttpGet("throw-and-catch-exception")]
        public IActionResult ThrowAndCatchException()
        {
            try
            {
                ThrowLevel1();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[Handled Exception] {ex.GetType().Name}: {ex.Message}");
                Console.WriteLine(ex.StackTrace);
                _logger.LogWarning(ex, ".NET handled exception test endpoint caught an exception");
            }

            return Ok(new
            {
                message = "Exception was thrown, caught, and printed to console.",
                handled = true
            });
        }

        private void ThrowLevel1()
        {
            ThrowLevel2();
        }

        private void ThrowLevel2()
        {
            ThrowLevel3();
        }

        private void ThrowLevel3()
        {
            throw new CustomTestException("This exception is thrown and handled in .NET test endpoint.");
        }
    }
}
