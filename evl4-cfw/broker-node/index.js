const fs = require('fs');
const mqtt = require('mqtt');

const coap = require('coap')
// need mqttServer, mqttUser, mqttPassword, mqttTopicPrefix.
const config = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));

const connectivityTopic = config.mqttTopicPrefix + "/connectivity";

const winston = require('winston');
 
const log = winston.createLogger({
  level: 'info',
  format: winston.format.simple(),
  transports: [
      new winston.transports.Console()
  ],
});

const client  = mqtt.connect(config.mqttServer, {
    username: config.mqttUser,
    password: config.mqttPassword,
    will: {
        topic: connectivityTopic,
        retain: true,
        payload: 'offline'
    }
});

client.on('connect', () => {
    log.info('Connected to broker');
});

const server = coap.createServer();

function heartbeatTimeout() {
    log.info('No heartbeat. Marking offline.');
    client.publish(connectivityTopic, 'offline', {retain: true});
    timeouter = setTimeout(heartbeatTimeout, 10 * 1000);
}

let timeouter = null
function kickHeartbeat() {
    log.info('Received heartbeat.');
    if(timeouter != null) {
        clearTimeout(timeouter)
    }
    
    client.publish(connectivityTopic, 'online', {retain: true});
    timeouter = setTimeout(heartbeatTimeout, 10 * 1000);
}
timeouter = setTimeout(heartbeatTimeout, 10 * 1000);

server.on('request', function (req, res) {
    if (req.url != '/publish') {
        res.code = 404;
        res.end();
        return;
    }
    console.log(req.payload.toString('hex'));
    console.log(req.url);
    if (req.payload.length % 2 != 0) {
        log.error('payload bad length', req.payload.toString('hex'));
        res.code = 400;
        res.end();
        return;
    }
    if (req.payload.length > 2) {
        // device has sent us a "heartbeat" message with all sensor data.
        kickHeartbeat();
    }
    
    for (let i = 0; i < req.payload.length; i+=2) {
        const sensorId = req.payload.readUInt8(i);
        const value = req.payload.readUInt8(i+1)
        client.publish(`${config.mqttTopicPrefix}/channels/${sensorId}`, value > 0 ? 'ON' : 'OFF', {retain: true});
    }
    
    res.end();
})

// the default CoAP port is 5683
server.listen(() => {
    log.info("CoAP listening on 5683");
});

