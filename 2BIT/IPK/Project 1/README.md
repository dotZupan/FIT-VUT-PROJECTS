# ipk-l4-scan

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [Theory Overview](#theory-overview)
3. [Functionalities](#functionalities)
4. [Testing Procedure](#testing-procedure)
5. [Extra Functionality](#extra-functionality)
6. [Bibliography](#bibliography)

## Executive Summary
The **ipk-l4-scan** application is a Layer 4 network scanner developed in C# (.NET 8+).
Its purpose is to perform both TCP and UDP scans on a specified target host, identifying
whether ports are open, closed, or filtered. The application supports both IPv4 and IPv6 and
uses raw sockets for TCP SYN scanning and ICMP messages for detecting closed UDP ports.

The program is compatible with Unix-based systems and is designed to run under elevated privileges
due to its reliance on raw socket operations.

---

## Theory Overview
The scanning techniques implemented in **ipk-l4-scan** include:
- **TCP SYN Scanning**: The scanner constructs and sends SYN packets to target ports, awaiting a response.
  If the port is open, a SYN-ACK packet is returned. If the port is closed, an RST packet is returned. If no response or other response types are received, the port is considered filtered. The scanner attempts to send the packet one more time for confirmation.
- **UDP Scanning**: A UDP packet is sent to the target port. If the port is closed, an ICMP
  "Port Unreachable" (type 3, code 3) message is returned. If the port is open or filtered, no response is typically received.

---

## Functionalities
### Command-Line Parsing (ArgumentParser Class)
This section of the code handles parsing command-line arguments, including target specification, network interface selection, protocol specification (TCP/UDP), and port ranges.

### Port Scanning (PortScanner Class)
The `PortScanner` class is responsible for initializing scanning tasks and running them concurrently for multiple targets and ports.

### TCP Scanner (TcpScanner Class)
The `TcpScanner` class performs TCP SYN scanning using raw sockets. It implements packet creation, checksum calculation, and socket communication for both IPv4 and IPv6.

### UDP Scanner (UdpScanner Class)
The `UdpScanner` class sends UDP packets and listens for ICMP "Port Unreachable" messages. It supports both IPv4 and IPv6, ensuring compatibility with different network environments.

---

## Testing
### Testing Environment
- **Operating System**: WSL2 Ubuntu 22 (Nix managed environment)
- **.NET Version**: 8+
- **Network Interfaces**: Ethernet (eth0), Tunnel (tun0), Local (lo)
- **Privileges**: Root (for raw socket operations)

### Testing Procedure
- The most commonly used programs for testing were Wireshark, Netcat, and nmap.
- During the testing phase, **Wireshark** provided clear and robust information about
  sent packets, which was helpful in debugging.
- **nmap** was mainly used to verify the validity of my outputs.
- **Netcat** was very helpful during the initial phases. I used it to open specific ports
  and test the functionalities of my program on them.
- Because I was developing on WSL Ubuntu, **IPv6 testing** had to be conducted using the `tun0` interface from **FIT.ovpn**.
- The primary addresses used for testing were: `www.fit.vut.cz`, `www.google.com`, and `merlin.fit.vutbr.cz`.


---

## Extra Functionality
- Additional information about interfaces (`IpAddress`, `MAC`) is displayed in the list of active interfaces.

---

## Bibliography
1. Official .NET 8+ Documentation: [https://learn.microsoft.com/en-us/dotnet/](https://learn.microsoft.com/en-us/dotnet/)
2. Raw Socket Programming Documentation: [https://man7.org/linux/man-pages/man7/raw.7.html](https://man7.org/linux/man-pages/man7/raw.7.html)
3. IETF RFC 793 - Transmission Control Protocol: [https://tools.ietf.org/html/rfc793](https://tools.ietf.org/html/rfc793)

