art_make(
  MODULE_LIBRARIES
    lardata_ArtDataHelper
    lardataobj_RecoBase
    ${MF_MESSAGELOGGER}
    
    ${FHICLCPP}
    cetlib_except
)

cet_test(HitCollectorTest HANDBUILT
  DATAFILES hitcollectioncreator_test.fcl
  TEST_EXEC lar
  TEST_ARGS --rethrow-all --config ./hitcollectioncreator_test.fcl
  )

install_headers()
install_fhicl()
install_source()

