package weather

import (
	"encoding/xml"
	"fmt"
	"net/http"
	"sort"
	"time"

	"golang.org/x/net/html/charset"
)

type EnvCanadaWeather struct {
	StationCode string
}

type conditionsDocument struct {
	Condition   string  `xml:"condition"`
	IconCode    string  `xml:"iconCode"`
	Temperature float32 `xml:"temperature"`
}

func (c conditionsDocument) toConditions(observedAt *time.Time) Conditions {
	return Conditions{
		ObservedAt:      observedAt,
		TextDescription: c.Condition,
		IconCode:        c.IconCode,
		Temperature:     c.Temperature,
	}
}

type hourlyForecast struct {
	conditionsDocument
	XMLName     xml.Name `xml:"hourlyForecast"`
	DateTimeUTC string   `xml:"dateTimeUTC,attr"`
}

func (h hourlyForecast) toConditions() (Conditions, error) {
	t, err := time.Parse("200601021504", h.DateTimeUTC)
	if err != nil {
		return Conditions{}, err
	}

	return h.conditionsDocument.toConditions(&t), nil
}

type currentConditions struct {
	conditionsDocument
	XMLName  xml.Name `xml:"currentConditions"`
	DateTime []struct {
		XMLName   xml.Name `xml:"dateTime"`
		Zone      string   `xml:"zone,attr"`
		TimeStamp string   `xml:"timeStamp"`
	} `xml:"dateTime"`
}

func (c currentConditions) toConditions() (Conditions, error) {

	var realTime *time.Time
	for _, v := range c.DateTime {
		if v.Zone == "UTC" {
			t, err := time.Parse("20060102150405", v.TimeStamp)
			if err == nil {
				realTime = &t
			}
		}
	}

	if realTime == nil {
		return Conditions{}, fmt.Errorf("failed to parse timestamp")
	}

	return c.conditionsDocument.toConditions(realTime), nil
}

type parsedDocument struct {
	XMLName             xml.Name          `xml:"siteData"`
	CurrentConditions   currentConditions `xml:"currentConditions"`
	HourlyForecastGroup struct {
		HourlyForecast []hourlyForecast `xml:"hourlyForecast"`
	} `xml:"hourlyForecastGroup"`
}

func (e *EnvCanadaWeather) FetchWeather() (*WeatherState, error) {
	resp, err := http.Get(fmt.Sprintf("https://dd.weather.gc.ca/citypage_weather/xml/%s.xml", e.StationCode))
	if err != nil {
		return nil, fmt.Errorf("failed fetching: %w", err)
	}

	if resp.StatusCode != 200 {
		resp.Body.Close()
		return nil, fmt.Errorf("failed fetching: %s", resp.Status)
	}

	parsed := &parsedDocument{}

	decoder := xml.NewDecoder(resp.Body)
	decoder.CharsetReader = charset.NewReaderLabel
	err = decoder.Decode(&parsed)
	resp.Body.Close()
	//err = xml.Unmarshal(bodyRead, parsed)
	if err != nil {
		return nil, fmt.Errorf("failed parsing body: %w", err)
	}

	current, err := parsed.CurrentConditions.toConditions()
	if err != nil {
		return nil, fmt.Errorf("failed parsing current conditions: %w", err)
	}

	hourly := make([]Conditions, 0)
	for _, v := range parsed.HourlyForecastGroup.HourlyForecast {
		hourlyConditions, err := v.toConditions()
		if err != nil {
			return nil, fmt.Errorf("failed parsing hourly conditions: %w", err)
		}
		hourly = append(hourly, hourlyConditions)
	}

	sort.Slice(hourly, func(i, j int) bool {
		return hourly[i].ObservedAt.Unix() < hourly[j].ObservedAt.Unix()
	})

	return &WeatherState{
		Current:   current,
		NextHours: hourly,
	}, nil
}
