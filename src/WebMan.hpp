#pragma once
#include <curl/curl.h>
#include <iostream>
#include <stdio.h>
using namespace std;

// Most of this was shamelessly yoinked from various tutorials/StackOverflow pages.
namespace WebMan {
	namespace {
		struct MemoryStruct {
			char *memory;
			size_t size;
		};

		CURL *request_handle;
		CURL *put_handle;
		struct MemoryStruct request_chunk;
		struct MemoryStruct put_chunk;
		curl_slist *headers;
	} 

	size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
		size_t realsize = size * nmemb;
		struct MemoryStruct *mem = (struct MemoryStruct *)userp;

		char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
		if (ptr == NULL) {
			throw "not enough memory (realloc returned NULL)\n";
		}

		mem->memory = ptr;
		memcpy(&(mem->memory[mem->size]), contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;

		return realsize;
	}

	void Init() {
		curl_global_init(CURL_GLOBAL_ALL);

		request_handle = curl_easy_init();
		curl_easy_setopt(request_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(request_handle, CURLOPT_WRITEDATA, (void *)&request_chunk);
		curl_easy_setopt(request_handle, CURLOPT_USERAGENT, "swrstoys-ingamehostlist");
		curl_easy_setopt(request_handle, CURLOPT_SSL_VERIFYPEER, false);

		put_handle = curl_easy_init();
		curl_easy_setopt(put_handle, CURLOPT_CUSTOMREQUEST, "PUT");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(put_handle, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(put_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(put_handle, CURLOPT_WRITEDATA, (void *)&put_chunk);
		curl_easy_setopt(put_handle, CURLOPT_USERAGENT, "swrstoys-ingamehostlist");
		curl_easy_setopt(put_handle, CURLOPT_SSL_VERIFYPEER, false);
	}

	string Request(const string &url) {
		string response;
		int response_code;
		request_chunk.memory = (char *)calloc(1, sizeof(char));
		request_chunk.size = 0;

		curl_easy_setopt(request_handle, CURLOPT_URL, &url[0]);
		CURLcode res = curl_easy_perform(request_handle);

		if (res != CURLE_OK) {
			free(request_chunk.memory);
			throw curl_easy_strerror(res);
		}

		curl_easy_getinfo(request_handle, CURLINFO_RESPONSE_CODE, &response_code);

		if (response_code != 200) {
			free(request_chunk.memory);
			throw response_code;
		}

		response = string(request_chunk.memory, request_chunk.memory + request_chunk.size);

		free(request_chunk.memory);
		return response;
	}

	string Put(const string &url, const string &data) {
		string response;
		put_chunk.memory = (char *)calloc(1, sizeof(char));
		put_chunk.size = 0;

		curl_easy_setopt(put_handle, CURLOPT_URL, &url[0]);
		curl_easy_setopt(put_handle, CURLOPT_POSTFIELDS, &data[0]);
		CURLcode res = curl_easy_perform(put_handle);

		if (res != CURLE_OK) {
			free(put_chunk.memory);
			return string("connection error: " + to_string(res));
		}

		response = string(put_chunk.memory, put_chunk.memory + put_chunk.size);
		
		free(put_chunk.memory);
		return response;
	}

	void Cleanup() {
		curl_slist_free_all(headers);
		curl_easy_cleanup(put_handle);
		curl_easy_cleanup(request_handle);
		curl_global_cleanup();
	}
}; 
