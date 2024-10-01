
#include "access_point.h"

static err_t tcp_close_client_connection(TCPConnect *con_state, tcp_pcb *client_pcb, err_t close_err)
{
    if (client_pcb)
    {
        assert(con_state && con_state->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK)
        {
            printf("close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (con_state)
        {
            delete con_state;
        }
    }
    return close_err;
}
void tcp_server_close(TCPServer *state)
{
    if (state->server_pcb)
    {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}
static err_t tcp_server_sent(void *arg, tcp_pcb *pcb, u16_t len)
{
    TCPConnect *con_state = (TCPConnect *)arg;
    // printf("tcp_server_sent %u\n", len);
    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len)
    {
        // printf("all done\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}
// Function to parse query string and return a map of key-value pairs
static std::map<std::string, std::string> extract_params(const std::string &params)
{
    std::map<std::string, std::string> params_map;
    std::stringstream ss(params);
    std::string token;

    while (std::getline(ss, token, '&'))
    {

        size_t pos = token.find('=');

        if (pos != std::string::npos)
        {
            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);
            printf("key = %s, value = %s\n", key.c_str(), value.c_str());
            params_map[key] = value;
        }
    }
    return params_map;
}
err_t tcp_server_recv(void *arg, tcp_pcb *pcb, pbuf *p, err_t err)
{
    TCPConnect *con_state = (TCPConnect *)arg;
    if (!p)
    {
        printf("connection closed\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    assert(con_state && con_state->pcb == pcb);
    if (p->tot_len > 0)
    {
        // printf("tcp_server_recv %d err %d\n", p->tot_len, err);
        // Copy the request into the buffer
        pbuf_copy_partial(p, con_state->headers, p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len, 0);

        // Handle GET request
        if (strncmp(HTTP_GET, con_state->headers, sizeof(HTTP_GET) - 1) == 0)
        {
            char *request = con_state->headers + sizeof(HTTP_GET); // + space
            printf("HTTP REQUEST\n--------------------\nGET %s", request);
            char *params = strchr(request, '?');
            if (params)
            {
                if (*params)
                {
                    char *space = strchr(request, ' ');
                    *params++ = 0;
                    if (space)
                    {
                        *space = 0;
                    }
                }
                else
                {
                    params = NULL;
                }
            }

            // Generate content
            con_state->result_len = handle_http(request, params, con_state->result, sizeof(con_state->result), *(con_state->settings));
            // printf("Request: %s?%s\n", request, params);
            // printf("Result: %d\n", con_state->result_len);

            // Check we had enough buffer space
            if (con_state->result_len > sizeof(con_state->result) - 1)
            {
                printf("Too much result data %d\n", con_state->result_len);
                return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
            }

            // Generate web page
            if (con_state->result_len > 0)
            {
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADERS,
                                                 200, con_state->result_len);
                if (con_state->header_len > sizeof(con_state->headers) - 1)
                {
                    printf("Too much header data %d\n", con_state->header_len);
                    return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
                }
            }
            else
            {
                // Send redirect
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT,
                                                 ipaddr_ntoa(con_state->gw));
                printf("Sending redirect %s", con_state->headers);
            }

            // Send the headers to the client
            con_state->sent_len = 0;
            err_t err = tcp_write(pcb, con_state->headers, con_state->header_len, 0);
            if (err != ERR_OK)
            {
                printf("failed to write header data %d\n", err);
                return tcp_close_client_connection(con_state, pcb, err);
            }

            // Send the body to the client
            if (con_state->result_len)
            {
                err = tcp_write(pcb, con_state->result, con_state->result_len, 0);
                if (err != ERR_OK)
                {
                    printf("failed to write result data %d\n", err);
                    return tcp_close_client_connection(con_state, pcb, err);
                }
            }
        }
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}
static err_t tcp_server_poll(void *arg, tcp_pcb *pcb)
{
    TCPConnect *con_state = (TCPConnect *)arg;
    // printf("tcp_server_poll_fn\n");
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect clent?
}
static void tcp_server_err(void *arg, err_t err)
{
    TCPConnect *con_state = (TCPConnect *)arg;
    if (err != ERR_ABRT)
    {
        // printf("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(con_state, con_state->pcb, err);
    }
}
static err_t tcp_server_accept(void *arg, tcp_pcb *client_pcb, err_t err)
{
    TCPServer *state = (TCPServer *)arg;
    if (err != ERR_OK || client_pcb == NULL)
    {
        printf("failure in accept\n");
        return ERR_VAL;
    }
    printf("client connected\n");

    // Create the state for the connection
    TCPConnect *con_state = new TCPConnect();
    if (!con_state)
    {
        printf("failed to allocate connect state\n");
        return ERR_MEM;
    }
    con_state->pcb = client_pcb; // for checking
    con_state->gw = &state->gw;
    con_state->settings = state->settings;

    // setup connection to client
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}
bool tcp_server_open(void *arg, const char *ap_name)
{
    TCPServer *state = (TCPServer *)arg;
    printf("starting server on port %d\n", TCP_PORT);

    tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb)
    {
        printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err)
    {
        printf("failed to bind to port %d\n", TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb)
    {
        printf("failed to listen\n");
        if (pcb)
        {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    printf("Try connecting to '%s' (press 'd' to disable access point)\n", ap_name);
    return true;
}

static tm string_to_tm(const std::string &date_str, const std::string &time_str)
{
    // date_str format should be yyyy-mm-dd
    // time_str format should be hh:mm:ss
    tm timestamp = {};
    std::string datetime_str = date_str + " " + time_str;
    std::istringstream ss(datetime_str);
    ss >> std::get_time(&timestamp, "%Y-%m-%d %H:%M:%S");
    return timestamp;
}

static int handle_http(const char *request, const char *params, char *result, size_t max_result_len, Settings &settings)
{
    // debug -----------------------
    printf("request is <%s>\n", request);
    printf("params is <%s>\n", params);
    // -----------------------------
    int len = 0;
    // if the user refering the page
    if (strncmp(request, PAGE_TITLE, sizeof(PAGE_TITLE) - 1) == 0)
    {
        // See if the user sent params
        if (params)
        {
            printf("\ngot params!\n\n");
            auto params_map = extract_params(params);
            settings.set_current_time(string_to_tm(params_map[PARAM_CURRENT_DATE], params_map[PARAM_CURRENT_TIME]));
            settings.set_start_time(string_to_tm(params_map[PARAM_START_DATE], params_map[PARAM_START_TIME]));
            settings.set_birthday_time(string_to_tm(params_map[PARAM_BIRTHDAY_DATE], params_map[PARAM_BIRTHDAY_TIME]));
            printf("current:\n\tdate is %s, time is %s\n", params_map[PARAM_CURRENT_DATE].c_str(), params_map[PARAM_CURRENT_TIME].c_str());
            printf("start:\n\tdate is %s, time is %s\n", params_map[PARAM_START_DATE].c_str(), params_map[PARAM_START_TIME].c_str());
            printf("birthday:\n\tdate is %s, time is %s\n", params_map[PARAM_BIRTHDAY_DATE].c_str(), params_map[PARAM_BIRTHDAY_TIME].c_str());
        }
        // Generate result
        len = snprintf(result, max_result_len, html_content);
    }
    // printf("result is <%s>\n", result);
    return len;
}
