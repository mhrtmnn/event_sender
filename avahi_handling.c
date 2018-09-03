#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include <avahi-core/core.h>
#include <avahi-core/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>


/**
 * NOTE:
 * Code based on avahi provided example at master/examples/core-browse-services.c
 */

/***********************************************************************************************************************
* GLOBAL DATA
***********************************************************************************************************************/
static AvahiServer *server = NULL;
static AvahiSimplePoll *simple_poll = NULL;


/***********************************************************************************************************************
* CALLBACKS
***********************************************************************************************************************/

/* Called whenever a service has been resolved successfully or timed out */
static void resolve_callback(AvahiSServiceResolver *r,
							 AVAHI_GCC_UNUSED AvahiIfIndex interface,
							 AVAHI_GCC_UNUSED AvahiProtocol protocol,
							 AvahiResolverEvent event,
							 const char *name,
							 const char *type,
							 const char *domain,
							 const char *host_name,
							 const AvahiAddress *address,
							 uint16_t port,
							 AvahiStringList *txt,
							 AvahiLookupResultFlags flags,
							 AVAHI_GCC_UNUSED void* userdata)
{
    assert(r);

	if (event == AVAHI_RESOLVER_FOUND) {
		char a[AVAHI_ADDRESS_STR_MAX], *t;

		fprintf(stdout, "(Resolver) Service '%s' of type '%s' in domain '%s':\n", name, type, domain);

		avahi_address_snprint(a, sizeof(a), address);
		t = avahi_string_list_to_string(txt);
		fprintf(stdout,
			"\t%s:%u (%s)\n"
			"\tTXT=%s\n"
			"\tcookie is %u\n"
			"\tis_local: %i\n"
			"\twide_area: %i\n"
			"\tmulticast: %i\n"
			"\tcached: %i\n",
			host_name, port, a,
			t,
			avahi_string_list_get_service_cookie(txt),
			!!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
			!!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
			!!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
			!!(flags & AVAHI_LOOKUP_RESULT_CACHED));

		avahi_free(t);
	} else {
		fprintf(stderr, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n",
			name, type, domain, avahi_strerror(avahi_server_errno(server)));
	}

    avahi_s_service_resolver_free(r);
}

/* Called whenever a new services becomes available on the LAN or is removed from the LAN */
static void browse_callback(AvahiSServiceBrowser *b,
							AvahiIfIndex interface,
							AvahiProtocol protocol,
							AvahiBrowserEvent event,
							const char *name,
							const char *type,
							const char *domain,
							AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
							void* userdata)
{
    AvahiServer *s = userdata;
    assert(b);

    switch (event) {
        case AVAHI_BROWSER_FAILURE:
            fprintf(stderr, "(Browser) %s\n", avahi_strerror(avahi_server_errno(server)));
            avahi_simple_poll_quit(simple_poll);
            return;
        case AVAHI_BROWSER_NEW:
            fprintf(stderr, "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);

            /**
			 * We ignore the returned resolver object. In the callback
			 * function we free it. If the server is terminated before
			 * the callback function is called the server will free
			 * the resolver for us.
			 */
            if (!(avahi_s_service_resolver_new(s, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, s)))
                fprintf(stderr, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_server_errno(s)));

            break;
        case AVAHI_BROWSER_REMOVE:
            fprintf(stderr, "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
            break;
        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            fprintf(stderr, "(Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
            break;
    }
}


/***********************************************************************************************************************
* IMPLEMENTATION OF EXPORTED FUNCTIONS
***********************************************************************************************************************/
int main_ava() {
    AvahiServerConfig config;
    AvahiSServiceBrowser *sb = NULL;
    int error;
    int ret = 1;

    /* Initialize the psuedo-RNG */
    srand(time(NULL));

    /* Allocate main loop object */
    if (!(simple_poll = avahi_simple_poll_new())) {
        fprintf(stderr, "Failed to create simple poll object.\n");
        goto fail;
    }

    /* Do not publish any local records */
    avahi_server_config_init(&config);
    config.publish_hinfo = 0;
    config.publish_addresses = 0;
    config.publish_workstation = 0;
    config.publish_domain = 0;

    /* Set a unicast DNS server (wide_area_servers field) for wide area DNS-SD */
    // avahi_address_parse("192.168.50.1", AVAHI_PROTO_UNSPEC, &config.wide_area_servers[0]);
    // config.n_wide_area_servers = 1;
    // config.enable_wide_area = 1;

    /* Allocate a new server */
    server = avahi_server_new(avahi_simple_poll_get(simple_poll), &config, NULL, NULL, &error);

    /* Free the configuration data */
    avahi_server_config_free(&config);

    /* Check wether creating the server object succeeded */
    if (!server) {
        fprintf(stderr, "Failed to create server: %s\n", avahi_strerror(error));
        goto fail;
    }

    /* Create the service browser */
    if (!(sb = avahi_s_service_browser_new(server, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_http._tcp", NULL, 0, browse_callback, server))) {
        fprintf(stderr, "Failed to create service browser: %s\n", avahi_strerror(avahi_server_errno(server)));
        goto fail;
    }

    /* Run the main loop */
    avahi_simple_poll_loop(simple_poll);

    ret = 0;

fail:
    /* Cleanup things */
    if (sb)
        avahi_s_service_browser_free(sb);

    if (server)
        avahi_server_free(server);

    if (simple_poll)
        avahi_simple_poll_free(simple_poll);

    return ret;
}