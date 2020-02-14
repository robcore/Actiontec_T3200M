/*
 *  Mapping of UID/GIDs to name and vice versa.
 *
 *  Copyright (c) 2002, 2003 The Regents of the University of
 *  Michigan.  All rights reserved.
 *
 *  Marius Aamodt Eriksen <marius@umich.edu>
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the University nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <net/net_namespace.h>
#include "idmap.h"
#include "nfsd.h"

/*
 * Turn off idmapping when using AUTH_SYS.
 */
static bool nfs4_disable_idmapping = true;
module_param(nfs4_disable_idmapping, bool, 0644);
MODULE_PARM_DESC(nfs4_disable_idmapping,
		"Turn off server's NFSv4 idmapping when using 'sec=sys'");

/*
 * Cache entry
 */

/*
 * XXX we know that IDMAP_NAMESZ < PAGE_SIZE, but it's ugly to rely on
 * that.
 */

#define IDMAP_TYPE_USER  0
#define IDMAP_TYPE_GROUP 1

struct ent {
	struct cache_head h;
	int               type;		       /* User / Group */
	uid_t             id;
	char              name[IDMAP_NAMESZ];
	char              authname[IDMAP_NAMESZ];
};

/* Common entry handling */

#define ENT_HASHBITS          8
#define ENT_HASHMAX           (1 << ENT_HASHBITS)

static void
ent_init(struct cache_head *cnew, struct cache_head *citm)
{
	struct ent *new = container_of(cnew, struct ent, h);
	struct ent *itm = container_of(citm, struct ent, h);

	new->id = itm->id;
	new->type = itm->type;

	strlcpy(new->name, itm->name, sizeof(new->name));
	strlcpy(new->authname, itm->authname, sizeof(new->name));
}

static void
ent_put(struct kref *ref)
{
	struct ent *map = container_of(ref, struct ent, h.ref);
	kfree(map);
}

static struct cache_head *
ent_alloc(void)
{
	struct ent *e = kmalloc(sizeof(*e), GFP_KERNEL);
	if (e)
		return &e->h;
	else
		return NULL;
}

/*
 * ID -> Name cache
 */

static struct cache_head *idtoname_table[ENT_HASHMAX];

static uint32_t
idtoname_hash(struct ent *ent)
{
	uint32_t hash;

	hash = hash_str(ent->authname, ENT_HASHBITS);
	hash = hash_long(hash ^ ent->id, ENT_HASHBITS);

	/* Flip LSB for user/group */
	if (ent->type == IDMAP_TYPE_GROUP)
		hash ^= 1;

	return hash;
}

static void
idtoname_request(struct cache_detail *cd, struct cache_head *ch, char **bpp,
    int *blen)
{
 	struct ent *ent = container_of(ch, struct ent, h);
	char idstr[11];

	qword_add(bpp, blen, ent->authname);
	snprintf(idstr, sizeof(idstr), "%u", ent->id);
	qword_add(bpp, blen, ent->type == IDMAP_TYPE_GROUP ? "group" : "user");
	qword_add(bpp, blen, idstr);

	(*bpp)[-1] = '\n';
}

static int
idtoname_upcall(struct cache_detail *cd, struct cache_head *ch)
{
	return sunrpc_cache_pipe_upcall(cd, ch, idtoname_request);
}

static int
idtoname_match(struct cache_head *ca, struct cache_head *cb)
{
	struct ent *a = container_of(ca, struct ent, h);
	struct ent *b = container_of(cb, struct ent, h);

	return (a->id == b->id && a->type == b->type &&
	    strcmp(a->authname, b->authname) == 0);
}

static int
idtoname_show(struct seq_file *m, struct cache_detail *cd, struct cache_head *h)
{
	struct ent *ent;

	if (h == NULL) {
		seq_puts(m, "#domain type id [name]\n");
		return 0;
	}
	ent = container_of(h, struct ent, h);
	seq_printf(m, "%s %s %u", ent->authname,
			ent->type == IDMAP_TYPE_GROUP ? "group" : "user",
			ent->id);
	if (test_bit(CACHE_VALID, &h->flags))
		seq_printf(m, " %s", ent->name);
	seq_printf(m, "\n");
	return 0;
}

static void
warn_no_idmapd(struct cache_detail *detail, int has_died)
{
	printk("nfsd: nfsv4 idmapping failing: has idmapd %s?\n",
			has_died ? "died" : "not been started");
}


static int         idtoname_parse(struct cache_detail *, char *, int);
static struct ent *idtoname_lookup(struct ent *);
static struct ent *idtoname_update(struct ent *, struct ent *);

static struct cache_detail idtoname_cache = {
	.owner		= THIS_MODULE,
	.hash_size	= ENT_HASHMAX,
	.hash_table	= idtoname_table,
	.name		= "nfs4.idtoname",
	.cache_put	= ent_put,
	.cache_upcall	= idtoname_upcall,
	.cache_parse	= idtoname_parse,
	.cache_show	= idtoname_show,
	.warn_no_listener = warn_no_idmapd,
	.match		= idtoname_match,
	.init		= ent_init,
	.update		= ent_init,
	.alloc		= ent_alloc,
};

static int
idtoname_parse(struct cache_detail *cd, char *buf, int buflen)
{
	struct ent ent, *res;
	char *buf1, *bp;
	int len;
	int error = -EINVAL;

	if (buf[buflen - 1] != '\n')
		return (-EINVAL);
	buf[buflen - 1]= '\0';

	buf1 = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf1 == NULL)
		return (-ENOMEM);

	memset(&ent, 0, sizeof(ent));

	/* Authentication name */
	if (qword_get(&buf, buf1, PAGE_SIZE) <= 0)
		goto out;
	memcpy(ent.authname, buf1, sizeof(ent.authname));

	/* Type */
	if (qword_get(&buf, buf1, PAGE_SIZE) <= 0)
		goto out;
	ent.type = strcmp(buf1, "user") == 0 ?
		IDMAP_TYPE_USER : IDMAP_TYPE_GROUP;

	/* ID */
	if (qword_get(&buf, buf1, PAGE_SIZE) <= 0)
		goto out;
	ent.id = simple_strtoul(buf1, &bp, 10);
	if (bp == buf1)
		goto out;

	/* expiry */
	ent.h.expiry_time = get_expiry(&buf);
	if (ent.h.expiry_time == 0)
		goto out;

	error = -ENOMEM;
	res = idtoname_lookup(&ent);
	if (!res)
		goto out;

	/* Name */
	error = -EINVAL;
	len = qword_get(&buf, buf1, PAGE_SIZE);
	if (len < 0)
		goto out;
	if (len == 0)
		set_bit(CACHE_NEGATIVE, &ent.h.flags);
	else if (len >= IDMAP_NAMESZ)
		goto out;
	else
		memcpy(ent.name, buf1, sizeof(ent.name));
	error = -ENOMEM;
	res = idtoname_update(&ent, res);
	if (res == NULL)
		goto out;

	cache_put(&res->h, &idtoname_cache);

	error = 0;
out:
	kfree(buf1);

	return error;
}


static struct ent *
idtoname_lookup(struct ent *item)
{
	struct cache_head *ch = sunrpc_cache_lookup(&idtoname_cache,
						    &item->h,
						    idtoname_hash(item));
	if (ch)
		return container_of(ch, struct ent, h);
	else
		return NULL;
}

static struct ent *
idtoname_update(struct ent *new, struct ent *old)
{
	struct cache_head *ch = sunrpc_cache_update(&idtoname_cache,
						    &new->h, &old->h,
						    idtoname_hash(new));
	if (ch)
		return container_of(ch, struct ent, h);
	else
		return NULL;
}


/*
 * Name -> ID cache
 */

static struct cache_head *nametoid_table[ENT_HASHMAX];

static inline int
nametoid_hash(struct ent *ent)
{
	return hash_str(ent->name, ENT_HASHBITS);
}

static void
nametoid_request(struct cache_detail *cd, struct cache_head *ch, char **bpp,
    int *blen)
{
 	struct ent *ent = container_of(ch, struct ent, h);

	qword_add(bpp, blen, ent->authname);
	qword_add(bpp, blen, ent->type == IDMAP_TYPE_GROUP ? "group" : "user");
	qword_add(bpp, blen, ent->name);

	(*bpp)[-1] = '\n';
}

static int
nametoid_upcall(struct cache_detail *cd, struct cache_head *ch)
{
	return sunrpc_cache_pipe_upcall(cd, ch, nametoid_request);
}

static int
nametoid_match(struct cache_head *ca, struct cache_head *cb)
{
	struct ent *a = container_of(ca, struct ent, h);
	struct ent *b = container_of(cb, struct ent, h);

	return (a->type == b->type && strcmp(a->name, b->name) == 0 &&
	    strcmp(a->authname, b->authname) == 0);
}

static int
nametoid_show(struct seq_file *m, struct cache_detail *cd, struct cache_head *h)
{
	struct ent *ent;

	if (h == NULL) {
		seq_puts(m, "#domain type name [id]\n");
		return 0;
	}
	ent = container_of(h, struct ent, h);
	seq_printf(m, "%s %s %s", ent->authname,
			ent->type == IDMAP_TYPE_GROUP ? "group" : "user",
			ent->name);
	if (test_bit(CACHE_VALID, &h->flags))
		seq_printf(m, " %u", ent->id);
	seq_printf(m, "\n");
	return 0;
}

static struct ent *nametoid_lookup(struct ent *);
static struct ent *nametoid_update(struct ent *, struct ent *);
static int         nametoid_parse(struct cache_detail *, char *, int);

static struct cache_detail nametoid_cache = {
	.owner		= THIS_MODULE,
	.hash_size	= ENT_HASHMAX,
	.hash_table	= nametoid_table,
	.name		= "nfs4.nametoid",
	.cache_put	= ent_put,
	.cache_upcall	= nametoid_upcall,
	.cache_parse	= nametoid_parse,
	.cache_show	= nametoid_show,
	.warn_no_listener = warn_no_idmapd,
	.match		= nametoid_match,
	.init		= ent_init,
	.update		= ent_init,
	.alloc		= ent_alloc,
};

static int
nametoid_parse(struct cache_detail *cd, char *buf, int buflen)
{
	struct ent ent, *res;
	char *buf1;
	int error = -EINVAL;

	if (buf[buflen - 1] != '\n')
		return (-EINVAL);
	buf[buflen - 1]= '\0';

	buf1 = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf1 == NULL)
		return (-ENOMEM);

	memset(&ent, 0, sizeof(ent));

	/* Authentication name */
	if (qword_get(&buf, buf1, PAGE_SIZE) <= 0)
		goto out;
	memcpy(ent.authname, buf1, sizeof(ent.authname));

	/* Type */
	if (qword_get(&buf, buf1, PAGE_SIZE) <= 0)
		goto out;
	ent.type = strcmp(buf1, "user") == 0 ?
		IDMAP_TYPE_USER : IDMAP_TYPE_GROUP;

	/* Name */
	error = qword_get(&buf, buf1, PAGE_SIZE);
	if (error <= 0 || error >= IDMAP_NAMESZ)
		goto out;
	memcpy(ent.name, buf1, sizeof(ent.name));

	/* expiry */
	ent.h.expiry_time = get_expiry(&buf);
	if (ent.h.expiry_time == 0)
		goto out;

	/* ID */
	error = get_int(&buf, &ent.id);
	if (error == -EINVAL)
		goto out;
	if (error == -ENOENT)
		set_bit(CACHE_NEGATIVE, &ent.h.flags);

	error = -ENOMEM;
	res = nametoid_lookup(&ent);
	if (res == NULL)
		goto out;
	res = nametoid_update(&ent, res);
	if (res == NULL)
		goto out;

	cache_put(&res->h, &nametoid_cache);
	error = 0;
out:
	kfree(buf1);

	return (error);
}


static struct ent *
nametoid_lookup(struct ent *item)
{
	struct cache_head *ch = sunrpc_cache_lookup(&nametoid_cache,
						    &item->h,
						    nametoid_hash(item));
	if (ch)
		return container_of(ch, struct ent, h);
	else
		return NULL;
}

static struct ent *
nametoid_update(struct ent *new, struct ent *old)
{
	struct cache_head *ch = sunrpc_cache_update(&nametoid_cache,
						    &new->h, &old->h,
						    nametoid_hash(new));
	if (ch)
		return container_of(ch, struct ent, h);
	else
		return NULL;
}

/*
 * Exported API
 */

int
nfsd_idmap_init(void)
{
	int rv;

	rv = cache_register_net(&idtoname_cache, &init_net);
	if (rv)
		return rv;
	rv = cache_register_net(&nametoid_cache, &init_net);
	if (rv)
		cache_unregister_net(&idtoname_cache, &init_net);
	return rv;
}

void
nfsd_idmap_shutdown(void)
{
	cache_unregister_net(&idtoname_cache, &init_net);
	cache_unregister_net(&nametoid_cache, &init_net);
}

static int
idmap_lookup(struct svc_rqst *rqstp,
		struct ent *(*lookup_fn)(struct ent *), struct ent *key,
		struct cache_detail *detail, struct ent **item)
{
	int ret;

	*item = lookup_fn(key);
	if (!*item)
		return -ENOMEM;
 retry:
	ret = cache_check(detail, &(*item)->h, &rqstp->rq_chandle);

	if (ret == -ETIMEDOUT) {
		struct ent *prev_item = *item;
		*item = lookup_fn(key);
		if (*item != prev_item)
			goto retry;
		cache_put(&(*item)->h, detail);
	}
	return ret;
}

static char *
rqst_authname(struct svc_rqst *rqstp)
{
	struct auth_domain *clp;

	clp = rqstp->rq_gssclient ? rqstp->rq_gssclient : rqstp->rq_client;
	return clp->name;
}

static __be32
idmap_name_to_id(struct svc_rqst *rqstp, int type, const char *name, u32 namelen,
		uid_t *id)
{
	struct ent *item, key = {
		.type = type,
	};
	int ret;

	if (namelen + 1 > sizeof(key.name))
		return nfserr_badowner;
	memcpy(key.name, name, namelen);
	key.name[namelen] = '\0';
	strlcpy(key.authname, rqst_authname(rqstp), sizeof(key.authname));
	ret = idmap_lookup(rqstp, nametoid_lookup, &key, &nametoid_cache, &item);
	if (ret == -ENOENT)
		return nfserr_badowner;
	if (ret)
		return nfserrno(ret);
	*id = item->id;
	cache_put(&item->h, &nametoid_cache);
	return 0;
}

static int
idmap_id_to_name(struct svc_rqst *rqstp, int type, uid_t id, char *name)
{
	struct ent *item, key = {
		.id = id,
		.type = type,
	};
	int ret;

	strlcpy(key.authname, rqst_authname(rqstp), sizeof(key.authname));
	ret = idmap_lookup(rqstp, idtoname_lookup, &key, &idtoname_cache, &item);
	if (ret == -ENOENT)
		return sprintf(name, "%u", id);
	if (ret)
		return ret;
	ret = strlen(item->name);
	BUG_ON(ret > IDMAP_NAMESZ);
	memcpy(name, item->name, ret);
	cache_put(&item->h, &idtoname_cache);
	return ret;
}

static bool
numeric_name_to_id(struct svc_rqst *rqstp, int type, const char *name, u32 namelen, uid_t *id)
{
	int ret;
	char buf[11];

	if (namelen + 1 > sizeof(buf))
		/* too long to represent a 32-bit id: */
		return false;
	/* Just to make sure it's null-terminated: */
	memcpy(buf, name, namelen);
	buf[namelen] = '\0';
#if !defined(CONFIG_BCM_KF_ANDROID) || !defined(CONFIG_BCM_ANDROID)
	ret = kstrtouint(name, 10, id);
#else
	ret = kstrtouint(buf, 10, id);
#endif
	return ret == 0;
}

static __be32
do_name_to_id(struct svc_rqst *rqstp, int type, const char *name, u32 namelen, uid_t *id)
{
	if (nfs4_disable_idmapping && rqstp->rq_flavor < RPC_AUTH_GSS)
		if (numeric_name_to_id(rqstp, type, name, namelen, id))
			return 0;
		/*
		 * otherwise, fall through and try idmapping, for
		 * backwards compatibility with clients sending names:
		 */
	return idmap_name_to_id(rqstp, type, name, namelen, id);
}

static int
do_id_to_name(struct svc_rqst *rqstp, int type, uid_t id, char *name)
{
	if (nfs4_disable_idmapping && rqstp->rq_flavor < RPC_AUTH_GSS)
		return sprintf(name, "%u", id);
	return idmap_id_to_name(rqstp, type, id, name);
}

__be32
nfsd_map_name_to_uid(struct svc_rqst *rqstp, const char *name, size_t namelen,
		__u32 *id)
{
	return do_name_to_id(rqstp, IDMAP_TYPE_USER, name, namelen, id);
}

__be32
nfsd_map_name_to_gid(struct svc_rqst *rqstp, const char *name, size_t namelen,
		__u32 *id)
{
	return do_name_to_id(rqstp, IDMAP_TYPE_GROUP, name, namelen, id);
}

int
nfsd_map_uid_to_name(struct svc_rqst *rqstp, __u32 id, char *name)
{
	return do_id_to_name(rqstp, IDMAP_TYPE_USER, id, name);
}

int
nfsd_map_gid_to_name(struct svc_rqst *rqstp, __u32 id, char *name)
{
	return do_id_to_name(rqstp, IDMAP_TYPE_GROUP, id, name);
}