package weather

import (
	"encoding/xml"
	"fmt"
	"net/http"

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

type hourlyForecast struct {
	conditionsDocument
	XMLName     xml.Name `xml:"hourlyForecast"`
	DateTimeUTC string   `xml:"dateTimeUTC,attr"`
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

	fmt.Printf("res: %+v\n", parsed)
	return nil, nil
}
