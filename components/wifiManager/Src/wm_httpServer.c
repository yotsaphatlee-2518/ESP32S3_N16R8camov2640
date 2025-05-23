/*!
* @file wm_httpServer.c
*
*	@date 2024
* @author Bulut Bekdemir
* 
* @copyright BSD 3-Clause License
* @version 0.4.6-prerelase.6+1
*/
#include "esp_log.h"
#include "esp_netif.h"

#include "wifiManager_private.h"
#include "wm_generalMacros.h"
#include "wm_httpServer.h"
#include "wm_wifi.h"

static const char *TAG = "WM_HTTP_SERVER";

/*!
* @brief HTTP Wifi Request Handler Semaphore
* @note This semaphore is used to handle the wifi request from the HTTP server.
*/
SemaphoreHandle_t wm_http_wifi_request_semaphore;

/*!
* @brief HTTP Server Task Handler
*
*/
httpd_handle_t wm_http_server_task_handle;

///> Declare the 503 response function
static void httpd_resp_send_503(httpd_req_t *req);

///> Embedded binary data for index.html, app.css, app.js, jquery-3.3.1.min.js and favicon.ico
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t scan_css_start[] asm("_binary_scan_css_start");
extern const uint8_t scan_css_end[] asm("_binary_scan_css_end");
extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");
extern const uint8_t password_html_start[] asm("_binary_password_html_start");
extern const uint8_t password_html_end[] asm("_binary_password_html_end");
extern const uint8_t password_css_start[] asm("_binary_password_css_start");
extern const uint8_t password_css_end[] asm("_binary_password_css_end");
extern const uint8_t password_js_start[] asm("_binary_password_js_start");
extern const uint8_t password_js_end[] asm("_binary_password_js_end");
extern const uint8_t jquery_3_3_1_min_js_start[] asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[] asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

/*!
* @brief Function Declarations for Wifi Scan Struct
*
*/
DECLARE_INIT_FUNC(wifi_app_wifi_scan_t);
DECLARE_DEINIT_FUNC(wifi_app_wifi_scan_t);
DECLARE_RETAIN_FUNC(wifi_app_wifi_scan_t);
DECLARE_RELEASE_FUNC(wifi_app_wifi_scan_t);


/*!
* @brief HTTP Server Index Handler
* @note Handles the index.html request
*
*/
static esp_err_t http_server_index_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
	return ESP_OK;
}

/*!
* @brief HTTP Server App CSS Handler
* @note Handles the app.css request
*
*/
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char *)scan_css_start, scan_css_end - scan_css_start);
	return ESP_OK;
}

/*!
* @brief HTTP Server App JS Handler
* @note Handles the app.js request
*
*/
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/javascript");
	httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);
	return ESP_OK;
}

/*!
* @brief HTTP Server Password HTML Handler
* @note Handles the password.html request
*
*/
static esp_err_t http_server_password_html_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *)password_html_start, password_html_end - password_html_start);
	return ESP_OK;
}

/*!
* @brief HTTP Server Password CSS Handler
* @note Handles the password.css request
*
*/
static esp_err_t http_server_password_css_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char *)password_css_start, password_css_end - password_css_start);
	return ESP_OK;
}

/*!
* @brief HTTP Server Password JS Handler
* @note Handles the password.js request
*
*/
static esp_err_t http_server_password_js_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/javascript");
	httpd_resp_send(req, (const char *)password_js_start, password_js_end - password_js_start);
	return ESP_OK;
}


/*!
* @brief HTTP Server Jquery Handler
* @note Handles the jquery-3.3.1.min.js request
*
*/
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);
	return ESP_OK;
}

/*!
* @brief HTTP Server Favicon Handler
* @note Handles the favicon.ico request
*
*/
static esp_err_t http_server_favicon_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);
	return ESP_OK;
}

/*!
* @brief HTTP Server Wifi Connect JSON Handler
* @note Handles the Wifi Connect request
*	@param req HTTP request
* @return ESP_OK 
*/
static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "Wifi Connect JSON Handler, waiting for semaphore");
	if(xSemaphoreTake(wm_http_wifi_request_semaphore, portMAX_DELAY) == pdTRUE)
	{
		if(!(xEventGroupGetBits(wm_http_event_group) & WM_EVENTG_HTTP_BLOCK_REQ) || !(xEventGroupGetBits(wm_http_event_group) & WM_EVENTG_HTTP_WIFI_CONNECTED))
		{
			ESP_LOGI(TAG, "Wifi Connect JSON Handler, semaphore taken");
			size_t lenSSID = 0, lenPassword = 0;
			char *ssid = NULL, *password = NULL;

			///> Get SSID header
			lenSSID = httpd_req_get_hdr_value_len(req, "ConnectSSID") + 1;
			if(lenSSID > 1)
			{
				ssid = malloc(lenSSID);
				if(httpd_req_get_hdr_value_str(req, "ConnectSSID", ssid, lenSSID) == ESP_OK)
				{
					ESP_LOGI(TAG, "SSID: %s", ssid);
				}
			}

			///> Get Password header
			lenPassword = httpd_req_get_hdr_value_len(req, "ConnectPassword") + 1;
			if(lenPassword > 1)
			{
				password = malloc(lenPassword);
				if(httpd_req_get_hdr_value_str(req, "ConnectPassword", password, lenPassword) == ESP_OK)
				{
					ESP_LOGI(TAG, "Password: %s", password);
				}
			}else
			{
				password = malloc(1);
				password[0] = '\0';
			}

			wifi_config_t* wifi_config = malloc(sizeof(wifi_config_t));
			memset(wifi_config, 0, sizeof(wifi_config_t));
			if (ssid && strlen(ssid) > 0) {
				strncpy((char*)wifi_config->sta.ssid, ssid, sizeof(wifi_config->sta.ssid) - 1);
				wifi_config->sta.ssid[sizeof(wifi_config->sta.ssid) - 1] = '\0';
			}
			if (password && strlen(password) > 0) {
				strncpy((char*)wifi_config->sta.password, password, sizeof(wifi_config->sta.password) - 1);
				wifi_config->sta.password[sizeof(wifi_config->sta.password) - 1] = '\0';
			}

			wm_wifi_send_message(wifi_config);

			xEventGroupSetBits(wm_wifi_event_group, WM_EVENTG_WIFI_CONNECT_FROM_HTTP);

			free(ssid);
			free(password);

		 	EventBits_t WifiStaBits =	xEventGroupWaitBits(wm_http_event_group, WM_EVENTG_HTTP_WIFI_CONNECTED | WM_EVENTG_HTTP_WIFI_CONNECT_FAIL | 
																										WM_EVENTG_HTTP_WIFI_AUTH_FAIL, pdFALSE, pdFALSE, portMAX_DELAY);
			if((WifiStaBits & WM_EVENTG_HTTP_WIFI_AUTH_FAIL) != 0)
			{
    		httpd_resp_set_type(req, "text/plain");
    		const char *resp_str = "Wrong SSID or Password, please try again.";
    		httpd_resp_send(req, resp_str, strlen(resp_str));
			}else if ((WifiStaBits & WM_EVENTG_HTTP_WIFI_CONNECTED) != 0)
			{
				httpd_resp_set_type(req, "text/plain");
				const char *resp_str = "Wifi Connected Successfully";
				httpd_resp_send(req, resp_str, strlen(resp_str));
			}else if ((WifiStaBits & WM_EVENTG_HTTP_WIFI_CONNECT_FAIL) != 0)
			{
				httpd_resp_set_type(req, "text/plain");
				const char *resp_str = "Wifi Connection Failed due to Unknown Error. Please try again.";
				httpd_resp_send(req, resp_str, strlen(resp_str));
			}
		}else 
		{
			httpd_resp_send_503(req);
		}
		xSemaphoreGive(wm_http_wifi_request_semaphore);
	}
	return ESP_OK;
}

/*!
* @brief HTTP Server Wifi Status JSON Handler
* @note Responses with the Wifi Connection status
*	@param req HTTP request
* @return ESP_OK
*/
static esp_err_t http_server_wifi_status_json_handler(httpd_req_t *req)
{
	char wifiJSON[192];
	char ip[16] = "", mask[16] = "", gw[16] = "";
	esp_netif_ip_info_t ip_info;
	esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
		sprintf(ip, IPSTR, IP2STR(&ip_info.ip));
		sprintf(mask, IPSTR, IP2STR(&ip_info.netmask));
		sprintf(gw, IPSTR, IP2STR(&ip_info.gw));
	}
	sprintf(wifiJSON, "{\"status\":%d, \"ip\":\"%s\", \"mask\":\"%s\", \"gw\":\"%s\"}",
		(xEventGroupGetBits(wm_http_event_group) & WM_EVENTG_HTTP_WIFI_CONNECTED ? 1 : 0),
		ip, mask, gw);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, wifiJSON, strlen(wifiJSON));
	return ESP_OK;
}

/*!
* @brief HTTP Server Wifi Scan Result List JSON Handler
* @note Responses with the Wifi Scan Result List
*	@param req HTTP request
* @return ESP_OK
*/
static esp_err_t http_server_wifi_scan_result_list_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "Wifi Scan Result List JSON Handler, waiting for semaphore");
	if(xSemaphoreTake(wm_http_wifi_request_semaphore, portMAX_DELAY) == pdTRUE)
	{
		if(!(xEventGroupGetBits(wm_http_event_group) & WM_EVENTG_HTTP_BLOCK_REQ))
		{
			ESP_LOGI(TAG, "Wifi Scan Result List JSON Handler, semaphore taken");
			xEventGroupSetBits(wm_wifi_event_group, WM_EVENTG_WIFI_SCAN_START);
			wifi_app_wifi_scan_t* wifi_scan = wifi_app_wifi_scan_t_init();
			wm_wifi_receive_scan_message(wifi_scan);
			char wifiScanJSON[1000];
			sprintf(wifiScanJSON, "{\"ap_count\":%d, \"ap_records\":[", wifi_scan->ap_count);
			ESP_LOGI(TAG, "Wifi Scan Result List JSON Handler, wifi_scan->ap_count: %d %s", wifi_scan->ap_count, wifi_scan->ap_records[0].ssid);
			for(int i = 0; i < wifi_scan->ap_count; i++)
			{
				char temp[100];
				sprintf(temp, "{\"ssid\":\"%s\", \"rssi\":%d, \"authmode\":%d}", wifi_scan->ap_records[i].ssid, wifi_scan->ap_records[i].rssi, wifi_scan->ap_records[i].authmode);
				strcat(wifiScanJSON, temp);
				if(i < wifi_scan->ap_count - 1)
				{
					strcat(wifiScanJSON, ",");
				}
			}
			strcat(wifiScanJSON, "]}");
			ESP_LOGI(TAG, "Wifi Scan Result List JSON Handler, wifiScanJSON: %s", wifiScanJSON);
			httpd_resp_set_type(req, "application/json");
			httpd_resp_send(req, wifiScanJSON, strlen(wifiScanJSON));

			wifi_app_wifi_scan_t_deinit(wifi_scan);
			xEventGroupSetBits(wm_wifi_event_group, WM_EVENTG_WIFI_SCAN_RESULT_SENT);		
			ESP_LOGI(TAG, "Wifi Scan Result List JSON Handler, semaphore to be given");
		}
		else
		{
			httpd_resp_send_503(req); //Default 503 response for blocking requests
		}	
		xSemaphoreGive(wm_http_wifi_request_semaphore);
	}
		return ESP_OK;
}


/*!
 * @brief HTTP Server Configuration
 * @note Sets up the default HTTP Server configuration
 * 
 * @return If successful http server instance, otherwise NULL 
 */
static httpd_handle_t http_server_configure(void)
{
	//Generate default configuration
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	config.core_id = HTTP_SERVER_TASK_CORE_ID;
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

	config.max_uri_handlers = 20;
	config.recv_wait_timeout = 10; //10 seconds
	config.send_wait_timeout = 10; //10 seconds

	wm_http_wifi_request_semaphore = xSemaphoreCreateBinary();
	if(wm_http_wifi_request_semaphore == NULL)
	{
			ESP_LOGE(TAG, "Semaphore creation failed");
	}
	xSemaphoreGive(wm_http_wifi_request_semaphore); 


	ESP_LOGI(TAG, "Starting HTTP Server on port: '%d' with task priority: '%d'", config.server_port, config.task_priority);

	///> Start the httpd server
	if(httpd_start(&wm_http_server_task_handle, &config) == ESP_OK)
	{
		///> Register URI handlers
		httpd_uri_t index_uri = {
			.uri = "/",
			.method = HTTP_GET,
			.handler = http_server_index_handler,
			.user_ctx = NULL
		};

		httpd_uri_t app_css_uri = {
			.uri = "/scan.css",
			.method = HTTP_GET,
			.handler = http_server_app_css_handler,
			.user_ctx = NULL
		};

		httpd_uri_t app_js_uri = {
			.uri = "/app.js",
			.method = HTTP_GET,
			.handler = http_server_app_js_handler,
			.user_ctx = NULL
		};

		httpd_uri_t password_html_uri = {
			.uri = "/password.html",
			.method = HTTP_GET,
			.handler = http_server_password_html_handler,
			.user_ctx = NULL
		};

		httpd_uri_t password_css_uri = {
			.uri = "/password.css",
			.method = HTTP_GET,
			.handler = http_server_password_css_handler,
			.user_ctx = NULL
		};

		httpd_uri_t password_js_uri = {
			.uri = "/password.js",
			.method = HTTP_GET,
			.handler = http_server_password_js_handler,
			.user_ctx = NULL
		};

		httpd_uri_t jquery_uri = {
			.uri = "/jquery-3.3.1.min.js",
			.method = HTTP_GET,
			.handler = http_server_jquery_handler,
			.user_ctx = NULL
		};

		httpd_uri_t favicon_uri = {
			.uri = "/favicon.ico",
			.method = HTTP_GET,
			.handler = http_server_favicon_handler,
			.user_ctx = NULL
		};

		///> Wifi Connect JSON handler
		httpd_uri_t wifi_connect_json = {
			.uri = "/connectWifi.json",
			.method = HTTP_POST,
			.handler = http_server_wifi_connect_json_handler,
			.user_ctx = NULL
		};

		///> Wifi Connect Status JSON handler
		httpd_uri_t wifi_connect_status_json = {
			.uri = "/wifiConnectStatus",
			.method = HTTP_POST,
			.handler = http_server_wifi_status_json_handler,
			.user_ctx = NULL
		};

		///>Wifi Scan Result List JSON handler
		httpd_uri_t wifi_scan_result_list_json = {
			.uri = "/listofScannedWifiNetworks",
			.method = HTTP_POST,
			.handler = http_server_wifi_scan_result_list_json_handler,
			.user_ctx = NULL
		};

		///> Register the URI handlers
		httpd_register_uri_handler(wm_http_server_task_handle, &index_uri);
		httpd_register_uri_handler(wm_http_server_task_handle, &app_css_uri);
		httpd_register_uri_handler(wm_http_server_task_handle, &app_js_uri);
		httpd_register_uri_handler(wm_http_server_task_handle, &password_html_uri);
		httpd_register_uri_handler(wm_http_server_task_handle, &password_css_uri);
		httpd_register_uri_handler(wm_http_server_task_handle, &password_js_uri);
		httpd_register_uri_handler(wm_http_server_task_handle, &jquery_uri);
		httpd_register_uri_handler(wm_http_server_task_handle, &favicon_uri);
		///> Register the URI handlers for Wifi Connect
		httpd_register_uri_handler(wm_http_server_task_handle, &wifi_connect_json);
		httpd_register_uri_handler(wm_http_server_task_handle, &wifi_connect_status_json);
		///> Register the URI handlers for Wifi Scan
		httpd_register_uri_handler(wm_http_server_task_handle, &wifi_scan_result_list_json);

		return wm_http_server_task_handle;
	}
	return NULL; //Only returns if failed to start the server
}

/*!
* @brief HTTP Server 503 Response
* @note Sends 503 Service Unavailable response
*
*/
static void httpd_resp_send_503(httpd_req_t *req)
{
    // Respond with 503 Service Unavailable
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Retry-After", "10"); // Client should retry after 10 seconds

    const char *resp_str = "Service temporarily unavailable. Please try again later.";
    httpd_resp_send(req, resp_str, strlen(resp_str));
}

/*!
 * @brief Initialize and Start the HTTP Server
 * 
 */
BaseType_t http_server_init(void)	
{
	if(wm_http_server_task_handle == NULL)
	{
		wm_http_server_task_handle = http_server_configure();
		if(wm_http_server_task_handle == NULL) 
		{
			ESP_LOGE(TAG, "Failed to start HTTP Server");
			return pdFAIL;
		}
		else
		{
			ESP_LOGI(TAG, "HTTP Server started successfully");
			return pdTRUE;
		}
	}
	ESP_LOGW(TAG, "HTTP Server is already started");
	return pdFAIL;
}

/*!
* @brief Stops HTTP Server 
* @note Stops the HTTP Server and deletes the Wifi Request Semaphore
*
* @return pdTRUE if successful, otherwise pdFALSE 
*/
BaseType_t http_server_stop(void)
{
	ESP_LOGI(TAG, "Deleting Semaphore ");
	if(wm_http_wifi_request_semaphore != NULL)
	{
		vSemaphoreDelete(wm_http_wifi_request_semaphore);
		wm_http_wifi_request_semaphore = NULL;
	}
	else
	{
		ESP_LOGW(TAG, "HTTP Wifi Request Semaphore is already deleted");
	}
	ESP_LOGI(TAG, "Stopping HTTP Server");
	if(wm_http_server_task_handle != NULL)
	{
		httpd_stop(wm_http_server_task_handle);
		wm_http_server_task_handle = NULL;
	}
	else
	{
		ESP_LOGW(TAG, "HTTP Server is already stopped");
	}
	if(wm_http_server_task_handle == NULL && wm_http_wifi_request_semaphore == NULL)
	{
		return pdTRUE;
	}
	return pdFALSE;
}