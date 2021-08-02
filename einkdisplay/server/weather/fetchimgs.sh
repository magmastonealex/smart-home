mkdir -p wximgs

rm -rf weather-icons
git clone https://github.com/erikflowers/weather-icons
pushd weather-icons/svg
inkscape -w 50 -h 50 wi-day-sunny.svg -o ../../wximgs/00.png
inkscape -w 50 -h 50 wi-day-cloudy.svg -o ../../wximgs/01.png
inkscape -w 50 -h 50 wi-day-cloudy.svg -o ../../wximgs/02.png
inkscape -w 50 -h 50 wi-day-cloudy.svg -o ../../wximgs/03.png
inkscape -w 50 -h 50 wi-day-cloudy.svg -o ../../wximgs/04.png
inkscape -w 50 -h 50 wi-day-cloudy.svg -o ../../wximgs/05.png
inkscape -w 50 -h 50 wi-day-showers.svg -o ../../wximgs/06.png
inkscape -w 50 -h 50 wi-day-rain-mix.svg -o ../../wximgs/07.png
inkscape -w 50 -h 50 wi-day-snow.svg -o ../../wximgs/08.png
inkscape -w 50 -h 50 wi-day-thunderstorm.svg -o ../../wximgs/09.png
inkscape -w 50 -h 50 wi-cloudy.svg -o ../../wximgs/10.png
inkscape -w 50 -h 50 wi-rain.svg -o ../../wximgs/11.png
inkscape -w 50 -h 50 wi-rain.svg -o ../../wximgs/12.png
inkscape -w 50 -h 50 wi-rain.svg -o ../../wximgs/13.png
inkscape -w 50 -h 50 wi-hail.svg -o ../../wximgs/14.png
inkscape -w 50 -h 50 wi-rain-mix.svg -o ../../wximgs/15.png
inkscape -w 50 -h 50 wi-snow.svg -o ../../wximgs/16.png
inkscape -w 50 -h 50 wi-snow.svg -o ../../wximgs/17.png
inkscape -w 50 -h 50 wi-snow.svg -o ../../wximgs/18.png
inkscape -w 50 -h 50 wi-thunderstorm.svg -o ../../wximgs/19.png
inkscape -w 50 -h 50 wi-fog.svg -o ../../wximgs/20.png
inkscape -w 50 -h 50 wi-fog.svg -o ../../wximgs/21.png
inkscape -w 50 -h 50 wi-day-cloudy.svg -o ../../wximgs/22.png
inkscape -w 50 -h 50 wi-fog.svg -o ../../wximgs/23.png
inkscape -w 50 -h 50 wi-fog.svg -o ../../wximgs/24.png
inkscape -w 50 -h 50 wi-snow-wind.svg -o ../../wximgs/25.png
inkscape -w 50 -h 50 wi-snowflake-cold.svg -o ../../wximgs/26.png
inkscape -w 50 -h 50 wi-hail.svg -o ../../wximgs/27.png
inkscape -w 50 -h 50 wi-rain.svg -o ../../wximgs/28.png
inkscape -w 50 -h 50 wi-night-clear.svg -o ../../wximgs/30.png
inkscape -w 50 -h 50 wi-night-partly-cloudy.svg -o ../../wximgs/31.png
inkscape -w 50 -h 50 wi-night-partly-cloudy.svg -o ../../wximgs/32.png
inkscape -w 50 -h 50 wi-night-partly-cloudy.svg -o ../../wximgs/33.png
inkscape -w 50 -h 50 wi-night-partly-cloudy.svg -o ../../wximgs/34.png
inkscape -w 50 -h 50 wi-night-partly-cloudy.svg -o ../../wximgs/35.png
inkscape -w 50 -h 50 wi-night-rain.svg -o ../../wximgs/36.png
inkscape -w 50 -h 50 wi-night-rain-mix.svg -o ../../wximgs/37.png
inkscape -w 50 -h 50 wi-night-snow.svg -o ../../wximgs/38.png
inkscape -w 50 -h 50 wi-night-thunderstorm.svg -o ../../wximgs/39.png
inkscape -w 50 -h 50 wi-snow-wind.svg -o ../../wximgs/40.png
inkscape -w 50 -h 50 wi-tornado.svg -o ../../wximgs/41.png
inkscape -w 50 -h 50 wi-tornado.svg -o ../../wximgs/42.png
inkscape -w 50 -h 50 wi-windy.svg -o ../../wximgs/43.png
inkscape -w 50 -h 50 wi-fire.svg -o ../../wximgs/44.png
inkscape -w 50 -h 50 wi-windy.svg -o ../../wximgs/45.png
inkscape -w 50 -h 50 wi-thunderstorm.svg -o ../../wximgs/46.png
inkscape -w 50 -h 50 wi-thunderstorm.svg -o ../../wximgs/47.png
inkscape -w 50 -h 50 wi-hurricane.svg -o ../../wximgs/48.png
popd
rm -rf weather-icons