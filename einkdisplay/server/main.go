package main

import (
	"einkserver/weather"
	"einkserver/widgets"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"

	"github.com/disintegration/imaging"
	"github.com/fogleman/gg"
)

func main() {

	configBytes, err := ioutil.ReadFile(os.Args[1])
	if err != nil {
		panic(err)
	}

	var configFile struct {
		Feeds              []string `json:"feeds"`
		WeatherStationCode string   `json:"stationCode"`
	}

	if err := json.Unmarshal(configBytes, &configFile); err != nil {
		panic(err)
	}

	http.HandleFunc("/getimg", func(w http.ResponseWriter, r *http.Request) {
		fmt.Println("Connected...")
		dc := gg.NewContext(480, 800)
		dc.SetRGB(1, 1, 1)
		dc.Clear()
		dc.SetRGB(0, 0, 0)

		ww := &widgets.WeatherWidget{
			Weather: &weather.EnvCanadaWeather{
				StationCode: configFile.WeatherStationCode,
			},
			Placement: &widgets.WidgetPlacement{
				X: 45,
				Y: 22,
				W: 425,
				H: 80,
			},
		}

		ww.Draw(dc)

		cw := &widgets.CalendarWidget{
			Placement: &widgets.WidgetPlacement{
				X: 42,
				Y: 125,
				W: 426,
				H: 900,
			},
			Events: configFile.Feeds,
		}

		cw.Draw(dc)

		img := dc.Image()
		flipimg := imaging.FlipH(img)
		imgres := make([]uint8, (480*800)/8)
		for y := 0; y < 800; y += 8 {
			for x := 0; x < 480; x++ {
				byt := uint8(0)
				for i := 0; i < 8; i++ {
					c := flipimg.At(x, y+i)
					r, g, b, _ := c.RGBA()
					if r > 127 || g > 127 || b > 127 {
						byt |= uint8(1 << (7 - i))
					}
				}
				imgres[(x*100)+(y/8)] = byt
			}
		}
		w.Header().Add("Content-Length", "48000")
		w.Header().Add("Content-Type", "application/binary")
		w.WriteHeader(200)

		w.Write(imgres)
	})

	log.Fatal(http.ListenAndServe(":8991", nil))
}
