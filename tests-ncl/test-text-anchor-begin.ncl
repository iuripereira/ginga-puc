<ncl>
  <head>
    <regionBase>
      <region id='reg' width='50%' height='50%'/>
      <region id='reg2' top="50%" width='50%' height='50%'/>
    </regionBase>
    <descriptorBase>
      <descriptor id='desc' region='reg'/>
      <descriptor id='desc2' region='reg2'/>
    </descriptorBase>
    <connectorBase>

      <causalConnector id="onBeginStart">
         <simpleCondition role="onBegin" />
         <simpleAction role="start" />
      </causalConnector>

       <causalConnector id="onBeginStop">
         <simpleCondition role="onBegin" />
         <simpleAction role="stop" />
      </causalConnector>

       <causalConnector id="onEndStop">
         <simpleCondition role="onEnd" />
         <simpleAction role="stop" />
      </causalConnector>

       <causalConnector id="onEndStart">
         <simpleCondition role="onEnd" />
         <simpleAction role="start" />
      </causalConnector>

      <causalConnector id="onKeySelectionStart">
        <connectorParam name="var"/>
        <simpleCondition role="onSelection" key="$var" />
        <simpleAction role="start" />
      </causalConnector>

    </connectorBase>
  </head>
  <body>
     <port id='start' component='text1'/>
     <media id='text1' src='samples/text.txt' descriptor='desc' >
         <area id="segText" begin="3s" />
         <area id="segText2" begin="5s" />
     </media>
    
     <media id='text2' src='samples/text2.txt' descriptor='desc2' />
     
    <link xconnector="onBeginStart">
        <bind role="onBegin" component="text1" interface="segText" />
        <bind role="start" component="text2"  />
     </link>
   
   <link xconnector="onBeginStop">
        <bind role="onBegin" component="text1" interface="segText2" />
        <bind role="stop" component="text2"  />
     </link>

  </body>
</ncl>
