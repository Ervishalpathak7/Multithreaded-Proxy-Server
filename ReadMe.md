# Multithreaded Proxy Server with Load Balancing and Caching

## 📌 Overview
This project is a high-performance **Proxy Server** built using **Golang, Redis, and Docker**. It efficiently handles multiple client requests, distributes traffic across backend servers, and optimizes responses through caching. The goal is to improve **scalability, performance, and security** for web applications.

## 🚀 Features
- **Goroutine-Based Concurrency** → Efficiently handles multiple connections.
- **Load Balancing** → Distributes traffic using **Round Robin / Least Connections** strategy.
- **Caching (Redis LRU)** → Stores frequently requested responses to reduce backend load.
- **Connection Pooling** → Maintains persistent connections for faster request forwarding.
- **Rate Limiting** → Prevents excessive requests from overloading the backend.
- **Logging & Monitoring** → Tracks request details, errors, and server health.
- **Timeout & Retry Mechanism** → Retries failed requests and handles timeouts gracefully.
- **Dockerized Deployment** → Easy containerization for scalable and portable deployment.

## 📂 Tech Stack
- **Golang** → Efficient concurrency & networking capabilities.
- **Redis** → Caching and rate limiting with LRU eviction strategy.
- **Docker** → Containerization for scalability & deployment.

## ⚙️ Architecture
```
Client → Proxy Server → Load Balancer → Backend Servers
        ⬆️              ⬇️  
       Redis Cache (for caching responses)
```
1. The proxy **intercepts client requests** and checks **Redis cache**.
2. If cache hit ✅ → Serve response from Redis.
3. If cache miss ❌ → Forward request to backend via **load balancer**.
4. Store response in **Redis** before sending it to the client.

## 🛠️ Setup & Installation
### Prerequisites
- Install **Golang** (>=1.18)
- Install **Docker & Docker Compose**
- Install **Redis**

### Clone the Repository
```sh
 git clone https://github.com/Ervishalpathak7/Reverse-Proxy-Server
 cd reverse-proxy-server
```

### Run with Docker
```sh
docker-compose up --build
```

### Run Locally (Without Docker)
```sh
go mod tidy
go run main.go
```

## 🔥 Future Enhancements
- **TLS Support** for secure HTTPS communication.
- **Advanced Caching Strategies** for better cache invalidation.
- **Web-Based Dashboard** for real-time monitoring.

## 📜 License
This project is licensed under the **MIT License**.

