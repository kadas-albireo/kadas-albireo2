# In a perfect world, this port would be a feature of py-opencv-python[contrib]
# But in a perfect world, we wouldn't have to deal with the topic of missing submodules in vcpkg ports

vcpkg_download_distfile(ARCHIVE
  URLS "https://files.pythonhosted.org/packages/a7/9e/7110d2c5d543ab03b9581dbb1f8e2429863e44e0c9b4960b766f230c1279/opencv_contrib_python-${VERSION}-cp37-abi3-win_amd64.whl"
  FILENAME "opencv_contrib_python-${VERSION}-cp37-abi3-win_amd64.whl"
  SHA512 36e2be241d3b525e8d35bdc6a7ee80cf31cbc3e190734970faa782601db4ff8f26775c8deaa2c083600ee4dbd2f8af5311904e91a0b508da4c0067d068808437
)

vcpkg_python_install_wheel(WHEEL "${ARCHIVE}")

vcpkg_python_test_import(MODULE "cv2")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
