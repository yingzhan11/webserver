# webserver
## Config
'
if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        std::cerr << "Failed to resolve hostname: " << hostname << std::endl;
        return -1;
    }
'



