#include <sys/param.h>
#include <sys/device.h>

static int pci_match(struct device *parent, void *match, void *aux);
static void pci_attach(struct device *parent, struct device *self, void *aux);

struct cfattach pci_ca = {
    0, pci_match, pci_attach, NULL
};

struct cfdriver pci_cd = {
	NULL, "pci", DV_DULL
};

static int
pci_match(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	return (1);
}

static void
pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
}
