
typedef
struct
{
	int is_available;
	int size;
} MCB, *MCB_P;

char *mem_start_p;
int max_mem;
int allocated_mem; /* this is the memory in use. */
int mcb_count;

char *heap_end;

MCB_P memallocate(MCB_P, int);

enum { NEW_MCB = 0, NO_MCB, REUSE_MCB };
enum { FREE, IN_USE };

void InitMem(char *ptr, int size_in_bytes) {
	max_mem = size_in_bytes;
	mem_start_p = ptr;
	mcb_count = 0;
	allocated_mem = 0;
	heap_end = mem_start_p + size_in_bytes;
}

void* myalloc(int elem_size) {
	MCB_P p_mcb;
	int flag = NO_MCB;
	p_mcb = (MCB_P)mem_start_p;
	int sz;
	sz = sizeof(MCB);
	if ((elem_size + sz)  > (max_mem - (allocated_mem + mcb_count * sz))) {
		return 0;
	}
	while (heap_end > ((char *)p_mcb + elem_size + sz))	{
		if (p_mcb->is_available == 0) {
			if (p_mcb->size == 0) {
				flag = NEW_MCB;
				break;
			}
			if (p_mcb->size > (elem_size + sz)) {
				flag = REUSE_MCB;
				break;
			}
		}
		p_mcb = (MCB_P)((char *)p_mcb + p_mcb->size);
	}
	if (flag != NO_MCB) {
		p_mcb->is_available = 1;
		if (flag == NEW_MCB) {
			p_mcb->size = elem_size + sizeof(MCB);
			mcb_count++;
		}
		allocated_mem += elem_size;
		return ((char *)p_mcb + sz);
	}
	return 0;
}

void myfree(void *p) {
	MCB_P ptr = (MCB_P)p;
	ptr--;
	mcb_count--;
	ptr->is_available = FREE;
	allocated_mem -= (ptr->size - sizeof(MCB));
}

unsigned char* __stdcall DetourGetFinalCode123(unsigned char* pbCode, int fSkipJmp)
{
	if (pbCode == 0) {
		return 0;
	}
	if (pbCode[0] == 0xff && pbCode[1] == 0x25) {
		pbCode = *(unsigned char* *)&pbCode[2];
		pbCode = *(unsigned char* *)pbCode;
	}
	else if (pbCode[0] == 0xe0 && fSkipJmp) {
		pbCode = pbCode + 5 + *(long*)&pbCode[1];
	}
	return pbCode;
}


unsigned char* __stdcall DetourFunc(unsigned char* pbTarget, unsigned char* pbDetour)
{
	unsigned char* pbTrampoline = myalloc(32);
	if (pbTrampoline == 0)
		return 0;
	//pbTarget = DetourGetFinalCode(pbTarget, 0);
	//pbDetour = DetourGetFinalCode(pbDetour, 0);
//	if (detour_insert_detour(pbTarget, pbTrampoline, pbDetour))
		return pbTrampoline;
	myfree(pbTrampoline);
	return 0;
}