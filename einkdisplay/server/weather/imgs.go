package weather

import (
	"bytes"
	"embed"
	"fmt"
	"image"

	_ "image/png" // register png
)

//go:embed wximgs/*
var weatherImages embed.FS

func (c Conditions) GetImageForConditions() (image.Image, error) {
	fetchStr := c.IconCode
	if fetchStr == "" {
		fetchStr = "00"
	}
	data, err := weatherImages.ReadFile(fmt.Sprintf("wximgs/%s.png", fetchStr))
	if err != nil {
		return nil, err
	}
	img, _, err := image.Decode(bytes.NewReader(data))
	if err != nil {
		return nil, err
	}
	return img, nil
}
