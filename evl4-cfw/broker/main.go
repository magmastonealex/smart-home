package main

// Super hacky CoAP -> MQTT gateway.
// There's lots of ways this could be better,
// but I don't have reason to expand it yet.
// If I ever add more tiny things runnning CoAP,
// then I'll expand to include subscribe support,
// multiple devices, etc.

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/plgd-dev/go-coap/v2"
	"github.com/plgd-dev/go-coap/v2/message"
	"github.com/plgd-dev/go-coap/v2/message/codes"
	"github.com/plgd-dev/go-coap/v2/mux"
)

type configInfo struct {
	MqttServer      string `json:"mqttServer"` //"tcp://10.102.40.20:1883"
	MqttUser        string `json:"mqttUser"`   // "videoloop"
	MqttPass        string `json:"mqttPassword"`
	MqttTopicPrefix string `json:"mqttTopicPrefix"` //"alexsroom/doorcam/motion" : ON
}

var globalConfig configInfo

const connectivityTopic string = "/connectivity"

func loadConfig() {
	fil, err := ioutil.ReadFile(os.Args[1])
	if err != nil {
		panic(err)
	}

	if err := json.Unmarshal(fil, &globalConfig); err != nil {
		panic(err)
	}

	if globalConfig.MqttServer == "" || globalConfig.MqttUser == "" || globalConfig.MqttPass == "" || globalConfig.MqttTopicPrefix == "" {
		panic(errors.New("Need to configure all mqtt variables"))
	}

}

var mqttClient mqtt.Client

var heartbeatChan chan<- struct{}

func handleA(w mux.ResponseWriter, req *mux.Message) {
	fullBody, err := io.ReadAll(req.Body)
	if err != nil {
		log.Printf("failed reading body: %+v", err)
		return
	}
	log.Printf("Got publish request! code: %+v body: %+v", req.Code, fullBody)

	if len(fullBody) > 2 {
		// device is alive! It's sent a heartbeat-style packet with the status of all channels.
		heartbeatChan <- struct{}{}
	}

	if len(fullBody)%2 != 0 {
		log.Printf("body of invalid length %d (%+v)", len(fullBody), fullBody)
		return
	}

	for i := 0; i < len(fullBody); i += 2 {
		channel := fullBody[i]
		value := fullBody[i+1]
		// Coerce value into on/off.
		sendVal := "ON"
		if value == 0x00 {
			sendVal = "OFF"
		}
		mqttClient.Publish(fmt.Sprintf("%s/channels/%d/status", globalConfig.MqttTopicPrefix, channel), 0, true, sendVal)
	}

	if req.IsConfirmable {
		err = w.SetResponse(codes.Changed, message.TextPlain, bytes.NewReader([]byte("")))
		if err != nil {
			log.Printf("cannot set response: %v", err)
			return
		}
	}
}

func main() {
	loadConfig()

	opts := mqtt.NewClientOptions().AddBroker(globalConfig.MqttServer)
	opts.SetKeepAlive(5 * time.Second)
	opts.SetPingTimeout(2 * time.Second)
	opts.SetAutoReconnect(true)
	opts.SetWill(globalConfig.MqttTopicPrefix+connectivityTopic, "offline", 0, true)
	opts.SetUsername(globalConfig.MqttUser)
	opts.SetPassword(globalConfig.MqttPass)
	opts.SetCleanSession(true)
	opts.SetClientID("coapbroker")

	hbChan := make(chan struct{}, 5)
	heartbeatChan = hbChan
	go func() {
		for {
			select {
			case <-hbChan:
				mqttClient.Publish(globalConfig.MqttTopicPrefix+connectivityTopic, 0, true, "online")
			case <-time.After(10 * time.Second):
				log.Println("No heartbeat after 10 seconds - marking offline.")
				mqttClient.Publish(globalConfig.MqttTopicPrefix+connectivityTopic, 0, true, "offline")
			}
		}
	}()

	// make sure the first connection succeeds.
	// After that auto-reconnect should deal with the rest.
	mqttClient = mqtt.NewClient(opts)
	if token := mqttClient.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	r := mux.NewRouter()
	r.Handle("/publish", mux.HandlerFunc(handleA))

	log.Fatal(coap.ListenAndServe("udp", ":5683", r))
}
