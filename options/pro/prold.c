         case F07 :
		   switch(choice(3,clearchoice))
		   {
		  case 1 :  if (sys.cell.row < 8 || sys.screen == SCREEN2) break;
			    wait();
			    clearrow();
				 calcall();
				 showall();
				 totals();
                              entercell(sys.cell);
			      break;
		   case 2 : wait();
			    switch( sys.cell.col)
			    {
                            case VOLC :
                            	case VALUEC :
				case VOLP :
				case DELTAC :
				case VALUEP :
                            	case DELTAP:
                            	case SHAREPRICE :break;
				case STOCKHELD : status.stockheld = 0; break;
				case VOLATILITY : status.volatility = 0.0;break;
				case INTEREST   : status.interest = 0.0; break;
				case  DATE      :
				case YEARMONTH :
				case SHARESPER : for (count =0;count <24; count++)
						   status.sizepay[count].sharesper = 1000;
						   break;               
				case EXPIRYDAY :
				case DAYSLEFT :
                            case MONTH : break;
                            case STRIKE : for (count = 0;count < 30; count++)
                                          {
					  status.data[count].strike = 0.0;
					  status.data[count].volc = 0.0;
					  status.data[count].volp = 0.0;
					  }
					  break;
				case MARKETC : for (count = 0;count < 30; count++)
                                        {
					status.data[count].marketc = 0.0;
					status.data[count].volc = 0.0;
					}
					  break;
				case HELDC :  for (count = 0;count < 30; count++)
					  status.data[count].heldc = 0;
					  break;
				case MARKETP : for (count = 0;count < 30; count++)
                                          {
					  status.data[count].marketp = 0.0;
					  status.data[count].volp = 0.0;
					  }
					  break;
				case HELDP :  for (count = 0;count < 30; count++)
					  status.data[count].heldp = 0;
					  break;
				case DIVIDENDDAY : for (count = 0;count < 24; count++)
					  status.sizepay[count].dday = 0;
					  break;
			        case DIVIDENDCENTS :  for (count = 0;count < 24; count++)
					  status.sizepay[count].payout = 0;
					  break;

			    }  wait();
				 calcall();
				 showall();
				 totals();  break;
			  case 3:   clearall();wait();   wait();
				 calcall();
				 showall();
				 totals();break;
			  /* clear all set shares per con */


		    }           showtotals();
				 ready();
				 entercell(sys.cell);
				 /* inverse vol now invalid */
		    break;