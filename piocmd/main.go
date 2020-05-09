package main

// Tool running on BeagleBone to talk to Pioneer TV over rs232 adapter.
// Could be an esp8266 just as easily, but I already have ethernet there,
// and the beaglebone is being used for other stuff anyway.

// TODO: Pull topics out into config file.

import (
	"encoding/json"
	"errors"
	"io"
	"io/ioutil"
	"log"
	"os"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/jacobsa/go-serial/serial"
)

type configInfo struct {
	MqttServer string `json:"mqttServer"` //"tcp://10.102.40.20:1883"
	MqttUser   string `json:"mqttUser"`   // "videoloop"
	MqttPass   string `json:"mqttPassword"`

	SerialPort string `json:"serialPort"`
	Baud       uint   `json:"baud"`
}

var globalConfig configInfo

func loadConfig() {

	fil, err := ioutil.ReadFile(os.Args[1])
	if err != nil {
		panic(err)
	}

	if err := json.Unmarshal(fil, &globalConfig); err != nil {
		panic(err)
	}

	if globalConfig.MqttServer == "" || globalConfig.MqttUser == "" || globalConfig.MqttPass == "" {
		panic(errors.New("must configure MQTT server"))
	}

	if globalConfig.SerialPort == "" || globalConfig.Baud == 0 {
		panic(errors.New("must configure serial port"))
	}

}

func pioCmd(command string, serialPort io.ReadWriteCloser) (string, error) {
	reqDeadline := time.Now().Add(4 * time.Second)

	respBuf := make([]byte, 100)
	resp := make([]byte, 1)

	reqBuf := make([]byte, 100)
	reqBuf[0] = 0x02
	reqBuf[1] = '*'
	reqBuf[2] = '*'
	copy(reqBuf[3:], []byte(command))
	reqBuf[3+len(command)] = 0x03

	log.Printf("req: %x", reqBuf[:3+len(command)+1])

	_, err := serialPort.Write(reqBuf[:3+len(command)+2])
	if err != nil {
		return "", err
	}
	log.Println("sent")

	//serialPort.SetReadDeadline(time.Now().Add(1 * time.Second))
	//defer serialPort.SetReadDeadline(time.Time{})

	bufIdx := 0

	for {
		n, err := serialPort.Read(resp)
		if err != nil && err != io.EOF {
			return "", err
		}
		if n != 0 {
			respBuf[bufIdx] = resp[0]
			bufIdx++
		}
		if resp[0] == 0x03 {
			// got end of msg!
			log.Printf("rx: %s\n", string(respBuf[1:bufIdx-1]))
			return string(respBuf[1 : bufIdx-1]), nil
		}

		if bufIdx == 99 {
			// msg too long... overflow.
			return "", errors.New("message overflowed")
		}

		if time.Now().After(reqDeadline) {
			return "", errors.New("deadline reached")
		}
	}

}

func handlePioneer(commandChan <-chan string, serialPort io.ReadWriteCloser, mqttClient mqtt.Client) {
	ticker := time.NewTicker(2 * time.Second)
	for {
		select {
		case <-ticker.C:
			var resp string
			var err error

			for i := 0; i < 3; i++ {
				resp, err = pioCmd("VOL", serialPort)
				if err != nil {
					log.Printf("Error: %+v", err)
					continue
				}
				break
			}

			if err != nil {
				panic(err)
			}

			if resp == "XXX" {
				mqttClient.Publish("media_player/alex/tv/state", 0, false, "OFF")
			} else {
				mqttClient.Publish("media_player/alex/tv/state", 0, false, "ON")
			}
		case cmd := <-commandChan:
			var err error

			for i := 0; i < 3; i++ {
				_, err = pioCmd(cmd, serialPort)
				if err != nil {
					log.Printf("Error: %+v", err)
					continue
				}
				break
			}

			if err != nil {
				panic(err)
			}
		}
	}
}

func main() {
	loadConfig()

	piocmdChan := make(chan string, 10)

	opts := mqtt.NewClientOptions().AddBroker(globalConfig.MqttServer)
	opts.SetAutoReconnect(true)
	opts.SetClientID("PioneerTvThing")
	opts.SetOnConnectHandler(func(c mqtt.Client) {
		log.Println("Connected!")
		token := c.Subscribe("media_player/alex/tv", 0, func(cl mqtt.Client, msg mqtt.Message) {
			cmd := string(msg.Payload())
			log.Printf("Got tv command: %s\n", cmd)

			if cmd == "ON" {
				piocmdChan <- string("PON")
			} else if cmd == "OFF" {
				piocmdChan <- string("POF")
			} else {
				piocmdChan <- string("PON")
				piocmdChan <- cmd
			}

		})

		if token.Wait() && token.Error() != nil {
			log.Fatal(token.Error())
		}

		token = c.Subscribe("media_player/alex/piocmd", 0, func(cl mqtt.Client, msg mqtt.Message) {
			log.Printf("Got cmd message: %s\n", string(msg.Payload()))
			// put in msg queue with nil callback.
			piocmdChan <- string(msg.Payload())
		})

		if token.Wait() && token.Error() != nil {
			log.Fatal(token.Error())
		}

		token = c.Subscribe("media_player/alex/volume", 0, func(cl mqtt.Client, msg mqtt.Message) {
			log.Printf("Got volume msg: %s\n", string(msg.Payload()))
			// put in msg queue with nil callback.
		})

		if token.Wait() && token.Error() != nil {
			log.Fatal(token.Error())
		}
	})

	opts.SetPassword(globalConfig.MqttPass)
	opts.SetUsername(globalConfig.MqttUser)

	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
	}

	options := serial.OpenOptions{
		PortName:              globalConfig.SerialPort,
		BaudRate:              globalConfig.Baud,
		DataBits:              8,
		StopBits:              1,
		MinimumReadSize:       0,
		InterCharacterTimeout: 100,
	}

	// Open the port.
	port, err := serial.Open(options)
	if err != nil {
		log.Panic(err)
	}

	// Make sure to close it later.
	defer port.Close()

	handlePioneer(piocmdChan, port, client)
}
