# ft_irc

**ft_irc** is a C++ implementation of an Internet Relay Chat (IRC) server. This project allows multiple clients to connect, create channels, chat in real-time, and manage channel modes, strictly following the protocols defined in the 42 School subject.

This project was developed to understand non-blocking I/O, socket programming, and network protocols using C++98.

## 🚀 Features

- **Non-blocking I/O:** Uses `select()` to handle multiple file descriptors simultaneously without threads.
- **Multi-client Support:** Handles multiple connections, disconnections, and data streams gracefully.
- **Channel Management:** Users can join, leave, and manage channels dynamically.
- **Operator Privileges:** Specific commands (KICK, INVITE, MODE) reserved for channel operators.
- **Strict Parsing:** Validates incoming commands, nicknames, and parameters.
- **Flood Control:** Uses buffering to handle partial commands and prevent data loss.

## 📥 Installation

To download and build the project, follow these steps:

1. **Clone the repository:**
   ```bash
   git clone [git@github.com:otboumeh/IRC.git](https://github.com/otboumeh/IRC)
   cd ft_irc
2. Compile the server: Run the following command to compile the source code:
   ```bash
      make
(This will generate the ircserv executable.)

💻 Usage

To start the server, you need to provide a listening port and a connection password.
   ```bash
   ./ircserv <port> <password>
```
- port: The port number on which the server will listen for incoming connections (must be between 1024 and 65535).

- password: The password required for clients to authenticate.

  Example:
```bash
   ./ircserv 6667 mysecretpassword
```
Once the server is running, you can connect to it using any IRC client (like Irssi, WeeChat, or NetCat) pointing to localhost (or your IP) on the specified port.

## 📡 Implemented Commands

The server supports the following standard IRC commands:

### Authentication & Connection
- `PASS <password>`: Sets the connection password. Must be sent before registration.
- `NICK <nickname>`: Sets or changes your nickname (max 9 chars).
- `USER <username> <mode> <unused> <realname>`: Registers the user connection details.
- `QUIT [message]`: Disconnects from the server with an optional quit message.
- `PING <token>`: Responds with a `PONG` to keep the connection alive.

### Channel Operations
- `JOIN <channel> [key]`: Joins a channel. If the channel requires a key (`+k`), it must be provided.
- `PART <channel> [reason]`: Leaves a specific channel.
- `TOPIC <channel> [topic]`: Views or changes the channel topic.
- `WHO <channel>`: Lists the members of a specific channel.
- `PRIVMSG <target> <text>`: Sends a private message to a user or a message to a channel.
- `NOTICE <target> <text>`: Similar to `PRIVMSG` but used for automatic replies/notifications (no errors returned).

### Operator / Moderation
- `KICK <channel> <user> [reason]`: Removes a user from the channel (Operator only).
- `INVITE <user> <channel>`: Invites a user to the channel (Required if channel is `+i`).
- `MODE`: Changes the mode of a channel.

  ## ⚙️ Channel Modes

The server supports the following modes via the `MODE` command:

| Flag | Description | Syntax |
| :--- | :--- | :--- |
| **i** | **Invite-only**: Only invited users can join. | `MODE #channel +i` |
| **t** | **Protected Topic**: Only operators can change the topic. | `MODE #channel +t` |
| **k** | **Key**: Sets a password for the channel. | `MODE #channel +k <password>` |
| **l** | **Limit**: Limits the number of users in the channel. | `MODE #channel +l <limit>` |
| **o** | **Operator**: Grants operator privilege to a user. | `MODE #channel +o <nickname>` |

*Example:*
```irc
MODE #42spain +o otboumeh
MODE #42spain +k secretpass
```
## 👥 Credits & Acknowledgments

A huge thank you to my teammate and collaborator:

<div align="center">

### **[veragarc](https://github.com/VeraGD)**

</div>

Thanks for the hard work, debugging sessions, and support in building this server! 🚀
