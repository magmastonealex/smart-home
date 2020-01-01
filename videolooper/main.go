package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"sync"
	"sync/atomic"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

var threadGroup sync.WaitGroup

var dumpMutex sync.Mutex
var endDumpingAt int64
var saveInProgress bool

type CircEntry struct {
	data   []byte
	used   bool
	length int
}

type circBuffer struct {
	sync.Mutex

	newInfoCond *sync.Cond

	entries      []*CircEntry
	totalEntries int
	readPos      int
	writePos     int

	numWritten uint64
}

type configInfo struct {
	MqttServer      string            `json:"mqttServer"`   //"tcp://10.102.40.20:1883"
	MqttUser        string            `json:"mqttUser"`     // "videoloop"
	MqttPass        string            `json:"mqttPassword"`
	MqttTopics      map[string]string `json:"mqttTopics"`   //"alexsroom/doorcam/motion" : ON
	HTTPPort        int               `json:"httpPort"`
	RtspURL         string            `json:"rtspUrl"` // rtsp://10.9.30.45:8554
	BufEntries      int               `json:"bufEntries"`
	SecondsRecord   int               `json:"secondsRecord"`
	RecordDirectory string            `json:"recordDirectory"`
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

	if globalConfig.BufEntries == 0 {
		globalConfig.BufEntries = 10000
	}
	if globalConfig.SecondsRecord == 0 {
		globalConfig.SecondsRecord = 20
	}

	if globalConfig.RtspURL == "" || (globalConfig.MqttServer != "" && (globalConfig.MqttUser == "" || globalConfig.MqttPass == "")) {
		panic(errors.New("Need to configure RTSP url"))
	}

	if globalConfig.MqttServer == "" && globalConfig.HTTPPort == 0 {
		panic(errors.New("Need to configure at least one trigger type"))
	}
}

// Read to be used when closely-following the Writer
func (cb *circBuffer) Read(buf []byte) int {
	cb.Lock()
	defer cb.Unlock()

	if cb.readPos == cb.totalEntries {
		fmt.Println("Rolling over...")
		cb.readPos = 0
	}

	for cb.readPos == cb.writePos {
		//fmt.Println("Waiting for more...")
		cb.newInfoCond.Wait()
		//fmt.Println("Got more?")
	}

	//fmt.Printf("Copying: %d %d %t", cb.readPos, cb.writePos, cb.entries[cb.readPos].used)
	copy(buf, cb.entries[cb.readPos].data)
	cb.entries[cb.readPos].used = false
	len := cb.entries[cb.readPos].length
	cb.readPos++
	return len
}

// Read to be used to read everything the writer has written that's in the buffer.
func (cb *circBuffer) ReadAll(callback func(buf []byte)) {
	cb.Lock()
	defer cb.Unlock()

	cb.readPos = cb.writePos + 1
	for cb.readPos != cb.writePos {
		if cb.readPos == cb.totalEntries {
			cb.readPos = 0
		}

		if cb.entries[cb.readPos].used == true {
			callback(cb.entries[cb.readPos].data[:cb.entries[cb.readPos].length])
			cb.entries[cb.readPos].used = false
		}

		cb.readPos++
	}
}

func (cb *circBuffer) Write(buf []byte) {
	cb.Lock()
	defer cb.Unlock()

	if cb.writePos == cb.totalEntries {
		cb.writePos = 0
	}

	atomic.AddUint64(&cb.numWritten, 1)
	n := copy(cb.entries[cb.writePos].data, buf)
	cb.entries[cb.writePos].used = true
	cb.entries[cb.writePos].length = n
	cb.writePos++
	cb.newInfoCond.Signal()
}

func (cb *circBuffer) watchdog() {
	ticker := time.NewTicker(5 * time.Second)
	for {
		<-ticker.C
		count := atomic.LoadUint64(&cb.numWritten)
		<-ticker.C
		count1 := atomic.LoadUint64(&cb.numWritten)
		if count1 == count {
			panic(errors.New("did not get write in 5 seconds"))
		}
	}
}

func writeToBuffer(r io.Reader, cb *circBuffer) {
	defer threadGroup.Done()
	buf := make([]byte, 1500)
	for {
		_, err := io.ReadFull(r, buf)
		if err != nil {
			panic(err)
		}
		cb.Write(buf)
	}

}

func dumpToFile(cb *circBuffer) {
	filename := fmt.Sprintf("%s/recording-%s.ts", globalConfig.RecordDirectory, time.Now().Format("2006-01-02T15_04_05"))

	f, err := os.OpenFile(filename, os.O_RDWR|os.O_CREATE, 0664)
	if err != nil {
		panic(err)
	}
	defer f.Close()

	buffer := bytes.NewBuffer([]byte{})
	cb.ReadAll(func(buf []byte) {
		buffer.Write(buf)
	})

	_, err = buffer.WriteTo(f)
	if err != nil {
		panic(err)
	}

	fmt.Println("Done catchup, continuing to dump for 30s...")

	buf := make([]byte, 1500)

	for {
		n := cb.Read(buf)
		_, err := f.Write(buf[:n])
		if err != nil {
			panic(err)
		}

		dumpMutex.Lock()
		now := time.Now().Unix()
		if now > endDumpingAt {
			break
		}
		dumpMutex.Unlock()
	}
	saveInProgress = false
	dumpMutex.Unlock()
}

func kickoffDump(cb *circBuffer) {
	dumpMutex.Lock()
	endDumpingAt = time.Now().Add(time.Duration(globalConfig.SecondsRecord) * time.Second).Unix()
	if saveInProgress == false {
		go dumpToFile(cb)
	}
	saveInProgress = true
	dumpMutex.Unlock()
}

func main() {
	loadConfig()

	var entrySlice []*CircEntry
	numEntries := globalConfig.BufEntries
	for i := 0; i < numEntries; i++ {
		entrySlice = append(entrySlice, &CircEntry{
			data: make([]byte, 1500),
		})
	}

	cb := &circBuffer{
		totalEntries: numEntries,
		entries:      entrySlice,
	}
	cb.newInfoCond = sync.NewCond(cb)

	// I'd love to use libavcodec directly for this, but it's not exposed nicely to Go.
	// Plus, it's annoying to use with this particular camera - ffmpeg does a good job
	// of masking the ugliness this camera spews.
	cmd := exec.Command("ffmpeg", "-loglevel", "error", "-i", globalConfig.RtspURL, "-map", "0:0", "-c:v", "copy", "-f", "mpegts", "-")

	stdout, err := cmd.StdoutPipe()
	if err != nil {
		log.Fatal(err)
	}

	go cb.watchdog()

	threadGroup.Add(1)
	go writeToBuffer(stdout, cb)

	stderr, err := cmd.StderrPipe()
	if err != nil {
		log.Fatal(err)
	}

	if err := cmd.Start(); err != nil {
		log.Fatal(err)
	}

	go io.Copy(os.Stderr, stderr)

	http.HandleFunc("/save", func(w http.ResponseWriter, r *http.Request) {
		kickoffDump(cb)

		fmt.Fprintf(w, "OK")

	})
	if globalConfig.MqttServer != "" {
		opts := mqtt.NewClientOptions().AddBroker(globalConfig.MqttServer)
		opts.SetAutoReconnect(true)
		opts.SetClientID("videolooper")
		opts.SetOnConnectHandler(func(c mqtt.Client) {
			for topic, value := range globalConfig.MqttTopics {
				fmt.Printf("Subscribing %s, listening for %s\n", topic, value)
				token := c.Subscribe(topic, 0, func(cl mqtt.Client, msg mqtt.Message) {
					fmt.Printf("Got msg on topic: %s\n", string(msg.Payload()))
					if string(msg.Payload()) == value {
						kickoffDump(cb)
					}
				})

				if token.Wait() && token.Error() != nil {
					log.Fatal(token.Error())
				}
			}

		})
		opts.SetPassword(globalConfig.MqttPass)
		opts.SetUsername(globalConfig.MqttUser)

		client := mqtt.NewClient(opts)
		if token := client.Connect(); token.Wait() && token.Error() != nil {
			log.Fatal(token.Error())
		}
	}

	if globalConfig.HTTPPort > 0 {
		log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", globalConfig.HTTPPort), nil))
	} else {
		select {}
	}
}
