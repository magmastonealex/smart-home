package main

import (
	"fmt"
	"encoding/json"
	"io/ioutil"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"time"
	"net/http"
)

type GlobalConfiguration struct {
	MqttTopics           []string            `json:"mqttTopics"`
	MqttUser	string `json:"mqttUser"`
	MqttPass	string `json:"mqttPass"`
	MqttServer	string `json:"mqttServer"`
}

var timestampMap map[string]int64
var mqttErrorChan chan error;
var mqttMsgChan chan mqtt.Message;

var globalConfig GlobalConfiguration

func credProvider() (string, string) {
	return globalConfig.MqttUser, globalConfig.MqttPass
}

func messageHandler(c mqtt.Client, msg mqtt.Message) {
	mqttMsgChan <- msg;
}

func connectHandler(client mqtt.Client) {

	for _,topic := range globalConfig.MqttTopics {
		token := client.Subscribe(topic, 0, messageHandler);
	    token.Wait();
	    if token.Error() != nil {
	    	panic(token.Error())
	    }
	}


}

func doHttp() {
	http.HandleFunc("/getInfo", func(w http.ResponseWriter, r *http.Request) {
		fmt.Printf("req for getInfo")
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(timestampMap)
	})
	http.ListenAndServe(":8473", nil);
}

func main() {
	timestampMap = make(map[string]int64)
	mqttErrorChan = make(chan error);
	mqttMsgChan = make(chan mqtt.Message)

	jsonConfig, err := ioutil.ReadFile("/config/config.json")
	if err != nil {
		panic("Can't read config.json!")
	}
	if err := json.Unmarshal(jsonConfig, &globalConfig); err != nil {
		panic(err)
		return
	}

	opts := mqtt.NewClientOptions().AddBroker(globalConfig.MqttServer).SetClientID("mqtt_cacher").SetCredentialsProvider(credProvider).SetAutoReconnect(true).SetCleanSession(true).SetOnConnectHandler(connectHandler)
	
	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
        panic(token.Error())
    }
    go doHttp()
    fmt.Printf("done, listening on 8473")

    for {
	msg := <-mqttMsgChan
	epochSeconds := time.Now().Unix()
	timestampMap[string(msg.Topic())] = epochSeconds
	fmt.Println("current: ", timestampMap)
    }
}
