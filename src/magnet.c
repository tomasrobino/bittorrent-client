#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "whole_bencode.h"
#include "magnet.h"

magnet_data* process_magnet(const char* magnet) {
    const int length = (int) strlen(magnet);
    int start = 4;
    Magnet_Attributes current_attribute = none;
    magnet_data* data = malloc(sizeof(magnet_data));
    // Initializing pointers to null
    data->xt=nullptr;
    data->dn=nullptr;
    data->tr=nullptr;
    data->ws=nullptr;
    data->as=nullptr;
    data->xs=nullptr;
    data->kt=nullptr;
    data->mt=nullptr;
    data->so=nullptr;

    //Initializing tracker linked list
    ll* head = malloc(sizeof(ll));
    head->next = nullptr;
    head->val = nullptr;
    ll* current = head;
    int tracker_count = 0;
    fprintf(stdout, "magnet data:\n");
    for (int i = 0; i <= length; ++i) {
        if (magnet[i] == '&' || magnet[i] == '?' || magnet[i] == '\0') {
            //Processing previous attribute
            if (current_attribute != none) {
                switch (current_attribute) {
                    case xt:
                        if (strncmp(magnet+start, "urn:btmh:", 9) == 0 || (strncmp(magnet+start, "urn:btih:", 9) == 0 && data->xt == nullptr)) { //SHA-1
                            data->xt = malloc(sizeof(char)*(i-start-9 +1));
                            strncpy(data->xt, magnet+start+9, i-start-9);
                            data->xt[i-start-9] = '\0';
                        } else {
                            fprintf(stderr, "Invalid URN");
                            exit(1);
                        }
                        fprintf(stdout, "xt:\n%s\n", data->xt);
                        break;
                    case dn:
                        data->dn = malloc(sizeof(char)*(i-start+1));
                        for (int j = 0; j < i-start; ++j) {
                            if (magnet[start+j] == '+') {
                                data->dn[j] = ' ';
                            } else data->dn[j] = magnet[start+j];
                        }
                        data->dn[i-start] = '\0';
                        fprintf(stdout, "dn:\n%s\n", data->dn);
                        break;
                    case xl:
                        data->xl = (int) decode_bencode_int(magnet+start, nullptr);
                        fprintf(stdout, "xl:\n%ld\n", data->xl);
                        break;
                    case tr:
                        if (tracker_count > 0) {
                            current->next = malloc(sizeof(ll));
                            current = current->next;
                            current->next = nullptr;
                        }
                        current->val = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        tracker_count++;
                        fprintf(stdout, "tr:\n%s\n", current->val);
                        break;
                    case ws:
                        data->ws = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        fprintf(stdout, "ws:\n%s\n", data->ws);
                        break;
                    case as:
                        data->as = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        fprintf(stdout, "as:\n%s\n", data->as);
                        break;
                    case xs:
                        data->xs = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        fprintf(stdout, "xs:\n%s\n", data->xs);
                        break;
                    case kt:
                        data->kt = malloc(sizeof(char)*(i-start+1));
                        for (int j = 0; j < i-start; ++j) {
                            if (magnet[start+j] == '+') {
                                data->kt[j] = ' ';
                            } else data->kt[j] = magnet[start+j];
                        }
                        data->kt[i-start] = '\0';
                        fprintf(stdout, "kt:\n%s\n", data->kt);
                        break;
                    case mt:
                        data->mt = curl_easy_unescape(nullptr, magnet+start, i-start, nullptr);
                        fprintf(stdout, "mt:\n%s\n", data->mt);
                        break;
                    case so:
                        data->so = malloc(sizeof(char)*(i-start+1));
                        strncpy(data->so, magnet+start, i-start);
                        data->so[i-start] = '\0';
                        break;
                    default: ;
                }
            }

            //Setting newly found attribute as current

            if (strncmp(magnet+i+1 , "xt", 2) == 0) {
                current_attribute = xt;
            } else if (strncmp(magnet+i+1 , "dn", 2) == 0) {
                current_attribute = dn;
            } else if (strncmp(magnet+i+1 , "xl", 2) == 0) {
                current_attribute = xl;
            } else if (strncmp(magnet+i+1 , "tr", 2) == 0) {
                current_attribute = tr;
            } else if (strncmp(magnet+i+1 , "ws", 2) == 0) {
                current_attribute = ws;
            } else if (strncmp(magnet+i+1 , "as", 2) == 0) {
                current_attribute = as;
            } else if (strncmp(magnet+i+1 , "xs", 2) == 0) {
                current_attribute = xs;
            } else if (strncmp(magnet+i+1 , "kt", 2) == 0) {
                current_attribute = kt;
            } else if (strncmp(magnet+i+1 , "mt", 2) == 0) {
                current_attribute = mt;
            } else if (strncmp(magnet+i+1 , "so", 2) == 0) {
                current_attribute = so;
            }
            start = i+4;
            i+=3;
        }
    }
    data->tr = head;
    return data;
}

void free_magnet_data(magnet_data* magnet_data) {
    if (magnet_data->xt != nullptr) free(magnet_data->xt);
    if (magnet_data->dn != nullptr) free(magnet_data->dn);
    while (magnet_data->tr != nullptr) {
        curl_free(magnet_data->tr->val);
        ll* next = magnet_data->tr->next;
        free(magnet_data->tr);
        magnet_data->tr = next;
    }
    if (magnet_data->ws != nullptr) curl_free(magnet_data->ws);
    if (magnet_data->as != nullptr) curl_free(magnet_data->as);
    if (magnet_data->xs != nullptr) curl_free(magnet_data->xs);
    if (magnet_data->kt != nullptr) free(magnet_data->kt);
    if (magnet_data->mt != nullptr) curl_free(magnet_data->mt);
    if (magnet_data->so != nullptr) free(magnet_data->so);
    if (magnet_data != nullptr) free(magnet_data);
}