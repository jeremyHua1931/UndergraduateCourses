//test sscpu
module sccomp_tb()                                            ;
    
   reg                  clk                                   ;
   reg                  rstn                                  ;
   
   integer              counter   =   0                       ;
      
   sccomp U_SCCOMP
   (
      .clk(clk), 
      .rstn(rstn) 
   );

   initial begin
      $readmemh( "test.data" , U_SCCOMP.U_IM.ROM); // load instructions into instruction memory
      
     
      clk            =         1                              ;
      rstn           =         1                              ;
      #5                                                      ;
      rstn           =         0                              ;
      #20                                                     ;
      rstn           =         1                              ;
     
   end
   
   always begin
      #(50)     clk  =   ~clk                                 ;   
      counter        =   counter  +  1                        ;
      //$display("PC = 0x%3X,  instr = 0x%8X,  NPC = 0x%3x,  line=%3d, counter=%3d", U_SCCOMP.PC, U_SCCOMP.instr,U_SCCOMP.U_SCPU.NPC,U_SCCOMP.PC/4+1,counter); // used for debug
      $monitor("PC = 0x%3X,  instr = 0x%8X,  NPC = 0x%3x,  line=%3d, counter=%3d", U_SCCOMP.PC, U_SCCOMP.instr,U_SCCOMP.U_SCPU.NPC,U_SCCOMP.PC/4+1,counter); // used for debug
      if( (U_SCCOMP.instr === 32'hxxxxxxxx) || (counter>=250))
        begin
         $stop                                                ;
        end                                               
  end //end always
   
endmodule
