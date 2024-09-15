#include <iostream>

#include <CertStore.hpp>
#include <HttpRequest.hpp>
#include <json.hpp>
#include <rsa.hpp>
#include <Server.hpp>
#include <ServerWebService.hpp>
#include <Socket.hpp>
#include <string.hpp>
#include <WebSocketMessage.hpp>

using namespace soup;

static Server serv;

struct Subscriber
{
	std::vector<std::string> subscriptions;

	void subscribe(std::string&& topic)
	{
		subscriptions.emplace_back(std::move(topic));
	}

	void unsubscribe(const std::string& topic)
	{
		for (auto i = subscriptions.begin(); i != subscriptions.end(); ++i)
		{
			if (*i == topic)
			{
				subscriptions.erase(i);
				break;
			}
		}
	}

	[[nodiscard]] bool isSubscribedTo(const std::string& topic) const noexcept
	{
		return std::find(subscriptions.begin(), subscriptions.end(), topic) != subscriptions.end();
	}
};

static void broadcast(const std::string& topic, const std::string& msg)
{
	std::string outgoing;
	outgoing.reserve(topic.size() + 1 + msg.size());
	outgoing.append(topic);
	outgoing.push_back(':');
	outgoing.append(msg);

	for (const auto& w : serv.workers)
	{
		if (w->type == WORKER_TYPE_SOCKET
			&& reinterpret_cast<Socket*>(w.get())->custom_data.isStructInMap(Subscriber)
			&& reinterpret_cast<Socket*>(w.get())->custom_data.getStructFromMapConst(Subscriber).isSubscribedTo(topic)
			)
		{
			ServerWebService::wsSendText(*reinterpret_cast<Socket*>(w.get()), outgoing);
		}
	}
}

static void handleRequest(Socket& s, HttpRequest&& req, ServerWebService&)
{
	/*if (req.path == "/pub")
	{
		if (auto jr = json::decode(req.body))
		{
			broadcast(jr->asObj().at("topic").asStr(), jr->asObj().at("msg").asStr());
			return ServerWebService::send204(s);
		}
		return ServerWebService::send400(s);
	}*/
	if (req.path.substr(0, 11) == "/pub?topic=")
	{
		if (!req.body.empty())
		{
			broadcast(req.path.substr(11), req.body);
			return ServerWebService::send204(s);
		}
		return ServerWebService::send400(s);
	}
	return ServerWebService::sendContent(s, "See https://pubsub.cat/ for usage information.");
}

int main()
{
	auto certstore = soup::make_shared<CertStore>();
	{
		X509Certchain certchain;
		SOUP_ASSERT(certchain.fromPem(string::fromFile("cert/fullchain.pem")));
		certstore->add(
			std::move(certchain),
			RsaPrivateKey::fromPem(string::fromFile("cert/privkey.pem"))
		);
	}

	ServerWebService web_srv{ &handleRequest };
	web_srv.should_accept_websocket_connection = [](Socket&, const HttpRequest& req, ServerWebService&)
	{
		return req.path == "/sub";
	};
	web_srv.on_websocket_message = [](WebSocketMessage& msg, Socket& s, ServerWebService&)
	{
		switch (msg.data.c_str()[0])
		{
		case '+':
			s.custom_data.getStructFromMap(Subscriber).subscribe(msg.data.substr(1));
			break;

		case '-':
			s.custom_data.getStructFromMap(Subscriber).unsubscribe(msg.data.substr(1));
			break;
		}
	};
	if (!serv.bind(80, &web_srv))
	{
		std::cout << "Failed to bind to port 80." << std::endl;
		return 1;
	}
	if (!serv.bindCrypto(443, &web_srv, std::move(certstore)))
	{
		std::cout << "Failed to bind to port 443." << std::endl;
		return 2;
	}
	std::cout << "Listening on ports 80 and 443." << std::endl;
	serv.run();
}
