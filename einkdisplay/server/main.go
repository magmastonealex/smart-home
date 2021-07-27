package main

import (
	"einkserver/weather"
	"fmt"
	"time"

	"github.com/fogleman/gg"
)

func main() {

	wxfetch := &weather.EnvCanadaWeather{
		StationCode: "ON/s0000573_e",
	}
	_, err := wxfetch.FetchWeather()
	if err != nil {
		panic(err)
	}

	days := []string{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}

	dc := gg.NewContext(480, 800)
	dc.SetRGB(1, 1, 1)
	dc.Clear()
	dc.SetRGB(0, 0, 0)

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

	dc.SavePNG("out.png")
}
