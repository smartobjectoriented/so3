#include <avz/fbdev_gnt.h>
#include <avz/domain.h>
#include <avz/memslot.h>
#include <avz/sched.h>

#define MAX_FBDEV_PFN 8

typedef struct {
	fbdev_info_t fbdev;
	addr_t fake_pfn;
	int current_slotID;
} fbdev_priv_t;

static fbdev_priv_t priv = {};

static void __map_fbdev(struct domain *d, const fbdev_info_t *pfn_info)
{
	size_t i;
	addr_t phys_addr;
	addr_t ipa_addr;
	size_t size;
	void *pgtable;

	pgtable = (void *)d->pagetable_vaddr;
	ipa_addr = d->fbdev_start_pfn << PAGE_SHIFT;

	for (i = 0; i < pfn_info->count; i++) {
		phys_addr = pfn_info->pfn[i] << PAGE_SHIFT;
		size = pfn_info->page_count[i] << PAGE_SHIFT;

		__create_mapping(pgtable, ipa_addr, phys_addr, size, true, S2);

		ipa_addr += size;
	}
}

static void __map_fake_fbdev(struct domain *d, addr_t fake_pfn,
			     const fbdev_info_t *real_fb)
{
	size_t i, j;
	addr_t phys_addr;
	addr_t ipa_addr;
	void *pgtable;

	pgtable = (void *)d->pagetable_vaddr;
	ipa_addr = d->fbdev_start_pfn << PAGE_SHIFT;
	phys_addr = fake_pfn << PAGE_SHIFT;

	for (i = 0; i < real_fb->count; i++) {
		for (j = 0; j < real_fb->page_count[i]; j++) {
			__create_mapping(pgtable, ipa_addr, phys_addr, PAGE_SIZE,
					 true, S2);

			ipa_addr += PAGE_SIZE;
		}
	}
}

void fbdev_set_pgtable(struct domain *d, int slotID)
{
	if (slotID <= 1) {
		return;
	}

	if (slotID == priv.current_slotID) {
		__map_fbdev(d, &priv.fbdev);
	} else {
		__map_fake_fbdev(d, priv.fake_pfn, &priv.fbdev);
	}
}

void fbdev_set_info(fbdev_info_t *fbdev, addr_t fake_pfn)
{
	memcpy(&priv.fbdev, fbdev, sizeof(*fbdev));
	priv.fake_pfn = fake_pfn;

	// TODO: check already existing capsule
}

void fbdev_change_focus(int new_slotID)
{
	if ((priv.current_slotID > 1) && memslot[priv.current_slotID].busy) {
		__map_fake_fbdev(domains[priv.current_slotID], priv.fake_pfn,
				 &priv.fbdev);
	}

	if ((new_slotID > 1) && memslot[new_slotID].busy) {
		__map_fbdev(domains[new_slotID], &priv.fbdev);
	}

	priv.current_slotID = new_slotID;
}

addr_t fbdev_get_addr(void)
{
	return current_domain->fbdev_start_pfn << PAGE_SHIFT;
}
