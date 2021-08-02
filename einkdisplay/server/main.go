package main

import (
	"einkserver/weather"
	"einkserver/widgets"
	"fmt"
	"time"

	"github.com/disintegration/imaging"
	"github.com/fogleman/gg"
)

func main() {

	//fmt.Printf("res: %+v\n", wx)

	days := []string{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}

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

	if err := dc.LoadFontFace("/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf", 30); err != nil {
		panic(err)
	}
	// Each day of week is 70 pixels wide.
	// We start at a fixed offset...

	now := time.Now()

	endOfMonth := time.Date(now.Year(), now.Month()+1, 0, 0, 0, 0, 0, time.UTC)
	//    startOfMonth := time.Date(now.Year(), now.Month(), 1, 0, 0, 0, 0, time.UTC)
	numDays := endOfMonth.Day()

	calBaseVidx := 300

	dc.DrawStringAnchored(fmt.Sprintf("%s %d", now.Month().String(), now.Year()), 240, float64(calBaseVidx), 0.5, 0.5)
	daysOffset := 10
	for idx, val := range days {
		dc.DrawString(val, float64(daysOffset)+float64((float64(idx)*65.71)), float64(calBaseVidx+40))
	}

	vidx := calBaseVidx + 70
	for i := 1; i <= numDays; i++ {
		today := time.Date(now.Year(), now.Month(), i, 0, 0, 0, 0, time.UTC)

		dayX := float64(daysOffset) + float64(32.7) + float64((float64(today.Weekday()) * 65.71))
		dayY := float64(vidx)

		if today.Day() == now.Day() {
			dc.SetRGB(0, 0, 0)
			dc.DrawRectangle(dayX-18, dayY-18, 36, 36)
			dc.Fill()
			dc.SetRGB(1, 1, 1)
		}
		dc.DrawStringAnchored(fmt.Sprintf("%d", i), dayX, dayY, 0.5, 0.5)

		if today.Day() == now.Day() {
			dc.SetRGB(0, 0, 0)
		}
		if today.Weekday() == 6 {
			vidx += 40
		}
	}

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
