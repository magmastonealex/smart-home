package main

import (
	"einkserver/weather"
	"einkserver/widgets"

	"github.com/disintegration/imaging"
	"github.com/fogleman/gg"
)

func main() {
	dc := gg.NewContext(480, 800)
	dc.SetRGB(1, 1, 1)
	dc.Clear()
	dc.SetRGB(0, 0, 0)

	ww := &widgets.WeatherWidget{
		Weather: &weather.EnvCanadaWeather{
			StationCode: "ON/s0000573_e",
		},
		Placement: &widgets.WidgetPlacement{
			X: 60,
			Y: 60,
			W: 350,
			H: 60,
		},
	}

	ww.Draw(dc)

	// Each day of week is 70 pixels wide.
	// We start at a fixed offset...

	cw := &widgets.CalendarWidget{
		Placement: &widgets.WidgetPlacement{
			X: 50,
			Y: 200,
			W: 350,
			H: 900,
		},
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

		http.HandleFunc("/carddata", func(w http.ResponseWriter, r *http.Request) {
			fmt.Println("Got card read")
			fmt.Println(r.URL.String())
			w.Header().Add("Content-Length", "2")
			w.Header().Add("Content-Type", "text/plain")
			w.WriteHeader(200)
			w.Write([]byte("OK"))
		})

		log.Fatal(http.ListenAndServe(":8080", nil))
	*/
}
