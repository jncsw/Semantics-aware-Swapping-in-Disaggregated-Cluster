use std::alloc::{Layout, System, GlobalAlloc};


struct CustomAllocator;

unsafe impl GlobalAlloc for CustomAllocator {
    unsafe fn alloc(&self, _layout: Layout) -> *mut u8 {
        let (layout, pos) = _layout.extend(Layout::new::<u64>()).unwrap();
        let ptr = System.alloc(layout.pad_to_align());
        let freq = ptr.offset(pos as isize);
        *freq = 0;
        ptr
    }
    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        let (layout, _) = layout.extend(Layout::new::<u64>()).unwrap();
        System.dealloc(ptr, layout.pad_to_align())
    }
}


#[global_allocator]
static A: CustomAllocator = CustomAllocator;


#[derive(Debug)]
pub struct A {
    name: String,
    val: i32,
    other_val: u64,
}

// TODO: these operations need to be incorporated with intermediate representation, 
// at location of read/write, this function increments hotness
fn increase_freq(a: &A) {
    let ptr = (a as *const A) as *mut u8;
    unsafe {
        let ptr = ptr.offset(Layout::new::<A>().size() as isize);
        *ptr += 1;
    }
}
// At time of physical page table swapping, retrieve hotness information
fn get_freq(a: &A) -> u64 {
    let ptr = (a as *const A) as *mut u8;
    unsafe {
        let ptr = ptr.offset(Layout::new::<A>().size() as isize) as *const u64;
        *ptr
    }
}

fn main() {
    let a = Box::new(A {name: "Hello".to_string(), val: 0i32, other_val: 0u64});
    println!("{:#?}", a);
    println!("Size: {}", Layout::new::<A>().size());
    increase_freq(&a); 
    increase_freq(&a); // Need integration with IR to identify instances of read/write access
    increase_freq(&a); // assuming a test case where "A" is accessed three times
    let freq = get_freq(&a); 
    println!("Current hotness of A: {}", freq); // result should be 3
}
