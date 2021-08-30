public class Hanoi {

  //Antall ringer, pinner
  public static void hanoi(int n, char fra, char hjelper, char til) {
    if (n <= 1) System.out.println(fra + "->" + til);
    else {
      hanoi(n-1, fra, til, hjelper);
      System.out.println(fra + "->" + til);
      hanoi(n-1, hjelper, fra, til);
    }
 }

 public static void main(String[] args) {
    int n = Integer.parseInt(args[0]);
    System.out.println("Tårnet i Hanoi");
    System.out.println("Flytte "+ n + " ring(er) fra pinne A til C, ved hjelp av B");
    hanoi(n, 'A', 'B', 'C');
  }

}
