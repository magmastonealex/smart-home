package weather

import (
	"embed"
	"image"
)

//go:embed wximgs/*
var weatherImages embed.FS

func (c Conditions) GetImageForConditions() (image.Image, error) {
	return nil, nil
}
