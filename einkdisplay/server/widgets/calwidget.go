package widgets

import (
	"fmt"
	"time"

	"github.com/fogleman/gg"
)

type CalendarWidget struct {
	Placement *WidgetPlacement
}

func (cw *CalendarWidget) Draw(dc *gg.Context) {
	days := []string{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}
	now := time.Now()

	txtSize := findTextPtSize(dc, (cw.Placement.W/7)-5, cw.Placement.H, "XXX")
	fmt.Println(txtSize)
	if err := dc.LoadFontFace("/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf", float64(txtSize)); err != nil {
		panic(err)
	}

	endOfMonth := time.Date(now.Year(), now.Month()+1, 0, 0, 0, 0, 0, time.UTC)
	//    startOfMonth := time.Date(now.Year(), now.Month(), 1, 0, 0, 0, 0, time.UTC)
	numDays := endOfMonth.Day()

	calBaseVidx := cw.Placement.Y

	dc.DrawStringAnchored(fmt.Sprintf("%s %d", now.Month().String(), now.Year()), float64(cw.Placement.X)+(float64(cw.Placement.W)/2), float64(calBaseVidx), 0.5, 0.5)
	daysOffset := cw.Placement.X + 10
	daysIncrement := cw.Placement.W / 7
	for idx, val := range days {
		dc.DrawString(val, float64(daysOffset)+float64((float64(idx)*daysIncrement)), float64(calBaseVidx+40))
	}

	vidx := calBaseVidx + 70
	for i := 1; i <= numDays; i++ {
		today := time.Date(now.Year(), now.Month(), i, 0, 0, 0, 0, time.UTC)

		dayX := float64(daysOffset) + float64(32.7) + float64((float64(today.Weekday()) * daysIncrement))
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
}
