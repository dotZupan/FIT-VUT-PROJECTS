# CHANGELOG

## Version 1.0.0 (Initial Release)
- Implemented basic TCP and UDP scanning functionality.
- Supported IPv4 and IPv6 addresses.
- Integrated argument parsing and user input validation.
- Developed raw socket handling for TCP scanning.
- Implemented ICMP message handling for UDP scanning.
- Added graceful termination handling with Ctrl+C.
- Implemented scope ID handling for IPv6 link-local addresses.
- Established Makefile for building the application.

---

## Known Limitations
- UDP scanning may produce false result if many ports are tested at the same time.
- Performance may be affected if scanning a large range of ports simultaneously.
- Raw sockets require elevated privileges to operate.
- Limited support for non-standard network interfaces.
- Weak parser error handling 

