package main

// TODOs:
//  - Home Assistant module
//  - Entity widgets for HA
//  - Alarm system widget
//  - MQTT connection
//    - Fire off RFIDs, receive refresh info. r

import (
	"einkserver/weather"
	"einkserver/widgets"
	"encoding/json"
	"io/ioutil"

	"github.com/disintegration/imaging"
	"github.com/fogleman/gg"
)

func main() {

	configBytes, err := ioutil.ReadFile("config.json")
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

	dc := gg.NewContext(480, 800)
	dc.SetRGB(1, 1, 1)
	dc.Clear()
	dc.SetRGB(0, 0, 0)

	ww := &widgets.WeatherWidget{
		Weather: &weather.EnvCanadaWeather{
			StationCode: configFile.WeatherStationCode,
		},
		Placement: &widgets.WidgetPlacement{
			X: 60,
			Y: 60,
			W: 350,
			H: 60,
		},
	}

	ww.Draw(dc)

	cw := &widgets.CalendarWidget{
		Placement: &widgets.WidgetPlacement{
			X: 50,
			Y: 200,
			W: 350,
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
			//fmt.Printf("%d %d %d\n", x, y/8, (x*100)+(y/8))
		}
	}

	dc.SavePNG("out.png")
	/*
		http.HandleFunc("/getimg", func(w http.ResponseWriter, r *http.Request) {
			fmt.Println("Connected...")
			w.Header().Add("Content-Length", "48000")
			w.Header().Add("Content-Type", "application/binary")
			w.WriteHeader(200)

			w.Write(imgres)
		})

		log.Fatal(http.ListenAndServe(":8080", nil))
	*/
}
