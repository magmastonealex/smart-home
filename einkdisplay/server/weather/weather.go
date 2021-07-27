package weather

import (
	"time"
)

type WeatherProvider interface {
	FetchWeather() (WeatherState, error)
}

type WeatherState struct {
	Current   Conditions
	NextHours []Conditions
}

type Conditions struct {
	ObservedAt      *time.Time
	TextDescription string
	IconCode        string
	Temperature     int
}
