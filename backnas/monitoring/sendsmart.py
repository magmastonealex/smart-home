from slack import WebClient
import sys 
sc = WebClient(token="{{slack_token}}")
def push(message):
        global sc
        sc.chat_postMessage(channel="#general", text="<!channel> "+message, username="SMART {{ansible_hostname}}",icon_emoji=":scream:")

push(sys.argv[1])
