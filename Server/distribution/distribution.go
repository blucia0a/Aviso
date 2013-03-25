package distribution

import( "math/rand"
        "log"
//        "fmt"
 //       "os"
  //      "time"
      )

type Distribution struct{

  bins []uint
  sum uint
  max uint

}


func (dist *Distribution) AppendIfNotPresent( initVals *[]float64 ){

  newBins := make( []uint, len(*initVals) - len( (*dist).bins ) )

  oldLen := len((*dist).bins)

  (*dist).bins = append( (*dist).bins, newBins... )

  for i := oldLen; i < len((*dist).bins); i++ {

    /*Element was not found*/
    rank := int((*initVals)[i] * 1000.0)
    if rank == 0{  rank = 1 }
    (*dist).sum += uint(rank)
    (*dist).bins[i] = uint(rank)
    if uint(rank) > (*dist).max{
      (*dist).max = uint(rank)
    }

  }



}

func NewDistribution( initVals *[]float64 ) *Distribution{

  var dist Distribution = Distribution{}

  dist.bins = make( []uint, len( *initVals ) )

  for f := range *initVals{

    rank := int((*initVals)[f] * 1000.0)
    if rank == 0{ rank = 1  }
    dist.sum += uint(rank)
    dist.bins[f] = uint(rank)
    if uint(rank) > dist.max{
     dist.max = uint(rank)
    }

  }
  return &dist

}

func (dist *Distribution) Draw( c chan uint ) {

  which := -1
  for {


      /*Drawee sends 0 if it wants data, and 1 if this dist should die*/
      exit := <-c
      if exit == 1{
        return
      }

      which = rand.Intn( int((*dist).sum) )
      sumSoFar := uint(0)
      for i := range (*dist).bins{

        f := (*dist).bins[i]
        sumSoFar += f
        if sumSoFar > uint(which) {

          c <- uint(i)
          break

        }

      }

  }
  /*Shouldn't ever get here*/
  return
}

func (dist *Distribution) Update( which uint, newNumber uint ){

  currentNum := dist.bins[int(which)]

  diff := int(newNumber) - int(currentNum)

  //fmt.Println("Distribution sum was ",(*dist).sum)
  (*dist).sum += uint(diff)
  if (*dist).sum <= 0{

    log.Fatal("[AVISO] distribution/distribution.go:97 - sum of distribution elements was <= 0\n")

  }
  //fmt.Println("Distribution sum is ",(*dist).sum)

  /*Note that if diff < 0, the bins are brought down*/
  (*dist).bins[int(which)] = newNumber
  if newNumber > (*dist).max{
    (*dist).max = newNumber
  }

/*  f,_ := os.Create("distro.out")
  fmt.Fprintf(f,"%v\n",time.Now())
  for i := range (*dist).bins {

    fmt.Fprintf(f, "%d(%d)\t|",i,(*dist).bins[i])

    rat := (float64((*dist).bins[i]) / float64((*dist).max)) * 80.0
    for j := 0; j < int(rat); j++{
      fmt.Fprintf(f,"=")
    }
    fmt.Fprintf(f,"\n")

  }

  f.Close()
*/

}
