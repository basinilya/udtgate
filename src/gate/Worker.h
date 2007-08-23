// Generic worker
class Worker {
public:
    Worker ();
    UDTSOCKET    peer_sock; /* for transit connection*/
};

/**
  Worker for input outside->local connections
*/

class InWorker : public Worker {
public:
    InWorker (UDTSOCKET sd);
private:
    TCPSOCKET client_sock;
};

/**
  Worker for output local->outside connections
*/
class OutWorker : public Worker {
public:
    OutWorker (int sd);
private:
    TCPSOCKET client_sock;
};

/**
  Worker for output transit connections
*/
class TransitWorker : public Worker {
    TransitWorker (UDTSOCKET sd);
private:
    UDTSOCKET client_sock;
};
