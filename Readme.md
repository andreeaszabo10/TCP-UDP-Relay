Copyright Szabo Andreea 2024-2025

# TCP-UDP Relay

This project is a client-server application designed to facilitate efficient and reliable message exchange over TCP and UDP protocols. It leverages I/O multiplexing using `select()` to manage multiple connections simultaneously, ensuring optimal performance.

## **Key Features**
- **Dual Protocol Support**: Handles both TCP and UDP communication for flexible message transmission.
- **Subscription System**: Clients can subscribe or unsubscribe to specific topics to filter the messages they receive.
- **Wildcard Support**: Advanced topic matching using `*` and `+` wildcards for dynamic subscription options.
- **Efficient Message Handling**: Sends message length alongside the actual data to ensure efficient and accurate data transmission.

## **Components**
- **Server**: Manages client connections, processes subscription commands, and forwards messages to subscribed clients.
- **Subscriber (Client)**: Connects to the server, subscribes/unsubscribes to topics, and receives messages accordingly.
- **Message Structures**:
  - `udp_msg`: Used for parsing incoming UDP messages, separating the topic, type, and content.
  - `tcp_msg`: Structures TCP messages with a length and data field to avoid redundant transmission.
  - `command_message`: Manages client commands like subscription, unsubscription, and client identification.

## **How It Works**
1. **Server Initialization**: The server starts by initializing TCP and UDP sockets and begins listening for incoming client connections and messages.
2. **Client Connection**:
   - Clients connect via TCP, sending identification and subscription commands.
   - UDP clients can send messages that the server then forwards to the appropriate TCP clients based on their subscriptions.
3. **Subscription Management**:
   - Clients can dynamically subscribe or unsubscribe from topics.
   - The server maintains a list of active subscriptions and ensures that only relevant messages are forwarded.
4. **Wildcard Matching**:
   - Supports advanced topic matching using wildcards (`*` and `+`), enabling clients to subscribe to topic patterns dynamically.
5. **Shutdown**:
   - The server and clients can issue an `exit` command to terminate the connection gracefully.

## **Usage**
1. **Start the Server**
```bash
./server <PORT>
```
2. **Start a Subscriber Client**
```bash
./subscriber <ID> <IP_SERVER> <PORT>
```
3. **Client Commands**:
   - `subscribe <TOPIC>` - Subscribe to a topic.
   - `unsubscribe <TOPIC>` - Unsubscribe from a topic.
   - `exit` - Exit the client application.
