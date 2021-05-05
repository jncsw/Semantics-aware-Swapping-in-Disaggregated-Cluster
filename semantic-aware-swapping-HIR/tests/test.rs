fn main() { 
  let mut a = 10;
  let mut arr = [0; 10];
  arr[2] = a; 
  let mut arr2 = [1; 20];
  arr2[5] = 8888;
  arr2[7] = 9999;
  a = 10;
  println!("{:?} {:?} {}", arr, arr2, a);
}