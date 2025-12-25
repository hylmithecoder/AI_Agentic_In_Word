invoke_url='https://integrate.api.nvidia.com/v1/chat/completions'

authorization_header='Authorization: Bearer $NVIDIA_API_KEY'
accept_header='Accept: application/json'
content_type_header='Content-Type: application/json'

data=$'{
  "model": "nvidia/nemotron-3-nano-30b-a3b",
  "messages": [
    {
      "role": "user",
      "content": "Hello i use arch btw"
    }
  ],
  "temperature": 1,
  "top_p": 1,
  "max_tokens": 16384,
  "seed": 42,
  "stream": true,
  "chat_template_kwargs": {
    "enable_thinking": true
  }
}'

response=$(curl --silent -i -w "\n%{http_code}" --request POST \
  --url "$invoke_url" \
  --header "$authorization_header" \
  --header "$accept_header" \
  --header "$content_type_header" \
  --data "$data"
)

echo "$response"