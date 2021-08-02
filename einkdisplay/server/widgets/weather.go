package widgets

import (
	"einkserver/weather"
	"fmt"

	"github.com/fogleman/gg"
	"github.com/nfnt/resize"
)

type WeatherWidget struct {
	Placement *WidgetPlacement
	Weather   weather.WeatherProvider
}

// TODO: Text size calculations might be a nice touch.
func (ww *WeatherWidget) Draw(dc *gg.Context) {
	dc.Push()
	defer dc.Pop()

	numFutureHours := 4

	boxWidth := ww.Placement.W / 6.0
	boxHeight := ww.Placement.H

	skipImgs := false

	if boxWidth <= 15 {
		skipImgs = true
	}
	imgSz := uint(boxWidth) - 15

	wx, err := ww.Weather.FetchWeather()
	if err != nil {
		panic(err)
	}

	// Weather...
	if !skipImgs {
		img, err := wx.Current.GetImageForConditions()
		if err != nil {
			panic(err)
		}
		img = resize.Resize(imgSz, imgSz, img, resize.Lanczos2)
		dc.DrawImage(img, int(ww.Placement.X)+13, int(ww.Placement.Y))
	}

	strlen := len(wx.Current.TextDescription)
	if strlen > 20 {
		strlen = 20
	}

	ptSz := findTextPtSize(dc, boxWidth*1.5, boxHeight, wx.Current.TextDescription[:strlen])
	if err := dc.LoadFontFace(FontFace, float64(ptSz)); err != nil {
		panic(err)
	}
	dc.DrawStringAnchored(wx.Current.TextDescription[:strlen], boxWidth+ww.Placement.X, ww.Placement.Y+boxHeight-15, 0.5, 0.5)
	dc.DrawRectangle(ww.Placement.X, ww.Placement.Y, boxWidth*2, boxHeight)
	dc.SetLineWidth(2)
	dc.Stroke()
	dc.SetLineWidth(1)

	ptSz = findTextPtSize(dc, boxWidth/1.2, boxHeight, "25.5")

	if err := dc.LoadFontFace(FontFace, float64(ptSz)); err != nil {
		panic(err)
	}

	dc.DrawStringAnchored(fmt.Sprintf("%.1f", wx.Current.Temperature), (boxWidth*1.5)+ww.Placement.X, (boxHeight/2.4)+ww.Placement.Y, 0.5, 0.5)

	hoursOffset := float64(boxWidth*2) + ww.Placement.X

	if boxWidth <= 20 {
		skipImgs = true
	}
	imgSz = uint(boxWidth) - 20
	for i := 1; i < (numFutureHours + 1); i++ {
		dc.DrawRectangle(hoursOffset, ww.Placement.Y, boxWidth, boxHeight)
		dc.SetLineWidth(2)
		dc.Stroke()
		dc.SetLineWidth(1)
		if !skipImgs {
			img, err := wx.NextHours[i].GetImageForConditions()
			if err != nil {
				panic(err)
			}

			img = resize.Resize(imgSz, imgSz, img, resize.Lanczos2)
			dc.DrawImage(img, int(hoursOffset+10), int(ww.Placement.Y))
		}

		dc.DrawStringAnchored(fmt.Sprintf("%.1f", wx.NextHours[i].Temperature), float64(hoursOffset+(boxWidth/2)), (boxHeight-10)+ww.Placement.Y, 0.5, 0.5)

		hoursOffset += boxWidth
	}
}
