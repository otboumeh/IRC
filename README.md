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