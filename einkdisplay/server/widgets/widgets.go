package widgets

import "github.com/fogleman/gg"

const FontFace string = "/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf"

type Widget interface {
	Draw(dc *gg.Context)
}

type WidgetPlacement struct {
	X float64
	Y float64
	W float64
	H float64
}

func RenderError(wp *WidgetPlacement, dc *gg.Context, err error) {

}

//"AAAAAAAAAAAAAAAAAAAA"

// Attempt to find the largest text size that is smaller or equal to desiredW/desiredH.
func findTextPtSize(dc *gg.Context, desiredW, desiredH float64, desiredStr string) int {
	dc.Push()
	defer dc.Pop()

	textSize := float64(100)
	for textSize > 1 {
		if err := dc.LoadFontFace(FontFace, textSize); err != nil {
			panic(err)
		}
		resW, resH := dc.MeasureString(desiredStr)
		if resW <= desiredW && resH <= desiredH {
			return int(textSize)
		}
		textSize--
	}
	return 1
}
