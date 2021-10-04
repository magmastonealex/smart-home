package widgets

import (
	"fmt"
	"math"
	"net/http"
	"sort"
	"time"

	"github.com/apognu/gocal"
	"github.com/fogleman/gg"
)

type CalendarWidget struct {
	Placement *WidgetPlacement
	Events    []string
}

func (cw *CalendarWidget) getFeedDetails() []gocal.Event {
	client := http.Client{
		Timeout: 5 * time.Second,
	}

	events := make([]gocal.Event, 0)

	for _, calURL := range cw.Events {
		start, end := time.Now(), time.Now().Add(35*24*time.Hour)
		resp, err := client.Get(calURL)
		if err != nil {
			panic(err)
		}
		if resp.StatusCode != 200 {
			panic(fmt.Errorf("Request failed with %d", resp.StatusCode))
		}
		parser := gocal.NewParser(resp.Body)
		parser.Start = &start
		parser.End = &end
		if err := parser.Parse(); err != nil {
			panic(err)
		}
		resp.Body.Close()

		for _, e := range parser.Events {
			events = append(events, e)

		}
	}
	sort.Slice(events, func(i, j int) bool {
		return events[i].Start.Before(*events[j].Start)
	})
	for _, e := range events {
		fmt.Printf("%s on %s by %+v\n", e.Summary, e.Start, e.Organizer)
	}

	return events
}

func dateEqual(date1, date2 time.Time) bool {
	y1, m1, d1 := date1.Date()
	y2, m2, d2 := date2.Date()
	return y1 == y2 && m1 == m2 && d1 == d2
}

func (cw *CalendarWidget) Draw(dc *gg.Context) {
	events := cw.getFeedDetails()

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

		// Look for matching events...
		hasEvent := false
		for _, e := range events {
			if dateEqual(*e.Start, today) {
				hasEvent = true
			}
		}

		if hasEvent {
			dc.SetRGB(0, 0, 0)
			dc.DrawCircle(dayX, dayY, 18)
			dc.Fill()
			dc.SetRGB(1, 1, 1)
			//fmt.Printf("Today %+v has event.\n", today)
		}

		dc.DrawStringAnchored(fmt.Sprintf("%d", i), dayX, dayY, 0.5, 0.5)

		if today.Day() == now.Day() || hasEvent {
			dc.SetRGB(0, 0, 0)
		}
		if today.Weekday() == 6 {
			vidx += 40
		}
	}
	vidx += 40

	txtSize = findTextPtSize(dc, (cw.Placement.W)-5, 40, "Jan 02 - Hello World This Is An Event S")
	if err := dc.LoadFontFace("/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf", float64(txtSize)); err != nil {
		panic(err)
	}
	evMax := int(math.Min(6, float64(len(events))))
	for _, e := range events[:evMax] {
		sumMax := int(math.Min(30, float64(len(e.Summary))))
		dc.DrawStringAnchored(fmt.Sprintf("%s - %s", e.Start.Format("Jan 02"), e.Summary[:sumMax]), cw.Placement.X+(cw.Placement.W/2), vidx, 0.5, 0.5)
		vidx += 40
	}

}
