public class Rekkesum {

	public static int rekkesum(int n) {
  	if (n == 1) return 1;
  	return rekkesum(n-1) + n;
	} 

 public static void main(String[] args) {
    int n = Integer.parseInt(args[0]);
		System.out.println(" " +  n);
    System.out.println(" ∑i = " + rekkesum(n));
		System.out.println("i=1");
  }

}
