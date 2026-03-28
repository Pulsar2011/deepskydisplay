#ifndef DSD_VERSION_H
#define DSD_VERSION_H

#include <string>
#include <vector>

namespace DSD
{

/**
 * @class gGIT DSD_version.h "DSTdisplay/DSD_version.h"
 * @brief Compile-time Git version information for the DSTdisplay library.
 *
 * @details Provides a singleton interface to the version string that is
 * embedded at build time from `git describe`. Callers use the static helper
 * methods to retrieve or compare version identifiers without instantiating
 * the class directly.
 *
 * Version strings follow the Git describe format:
 * @code
 *   v<MAJOR>.<MINOR>.<PATCH>[-<N>-g<SHA1>]
 *   e.g. "v1.1.0" or "v1.1.0-11-gbaf8c11"
 * @endcode
 *
 * @par Thread safety
 * The singleton is lazily initialised. Concurrent first-access from multiple
 * threads is not protected; applications should call any static method at
 * program start on the main thread.
 */
  class gGIT
  {

#ifdef _gTEST_version_
  friend class version_Tester;
  FRIEND_TEST(version_Tester, test_getVersion);
  FRIEND_TEST(version_Tester, test_readVersion);
  FRIEND_TEST(version_Tester, test_private);
  FRIEND_TEST(version_Tester, test_operator_newer);
  FRIEND_TEST(version_Tester, test_operator_older);
#endif

  private:
   const std::string _VERSION;       ///< Full version string, e.g. "v1.1.0-11-gbaf8c11"
   const std::string _VERSION_SHORT; ///< Short version string, e.g. "v1.1.0"
   const std::string _VERSION_SHA1;  ///< SHA1 suffix only, e.g. "11-gbaf8c11"

   ~gGIT();
   gGIT();

   static gGIT *this_ver; ///< Singleton instance (lazily created on first use)

   /**
    * @brief Parse a full version string into its numeric components.
    * @return Vector of {major, minor, patch} as @c int16_t values.
    */
   std::vector<int16_t> getVersion() const;

   /**
    * @brief Parse an arbitrary version string @p ver into {major, minor, patch}.
    * @param ver Version string in the format "vMAJOR.MINOR.PATCH[…]".
    * @return Vector of @c int16_t numeric components.
    */
   std::vector<int16_t> readVersion(const std::string& ver) const;

  public:
    /**
     * @brief Return the full version string embedded at build time.
     * @return String of the form "vMAJOR.MINOR.PATCH-N-gSHA1".
     */
    static const std::string version();

    /**
     * @brief Return the short version string (tag only, no SHA1).
     * @return String of the form "vMAJOR.MINOR.PATCH".
     */
    static const std::string version_short();

    /**
     * @brief Return the Git SHA1 / commit-distance suffix.
     * @return String of the form "N-gSHA1", or an empty string on a clean tag.
     */
    static const std::string sha1();

    /**
     * @brief Test whether the full version string matches @p ver exactly.
     * @param ver Expected full version string.
     * @return @c true if equal, @c false otherwise.
     */
    static bool checkVersion(const std::string& ver) {if(this_ver == NULL)gGIT::this_ver = new gGIT();return (ver.compare(this_ver->_VERSION)==0);}

    /**
     * @brief Test whether the short version string matches @p ver exactly.
     * @param ver Expected short version string (e.g. "v1.1.0").
     * @return @c true if equal, @c false otherwise.
     */
    static bool checkVersionShort(const std::string& ver) {if(this_ver == NULL)gGIT::this_ver = new gGIT();return (ver.compare(this_ver->_VERSION_SHORT)==0);}

    /**
     * @brief Test whether the SHA1 suffix matches @p ver exactly.
     * @param ver Expected SHA1 suffix string.
     * @return @c true if equal, @c false otherwise.
     */
    static bool checkSHA1(const std::string& ver) {if(this_ver == NULL)gGIT::this_ver = new gGIT();return (ver.compare(this_ver->_VERSION_SHA1)==0);}

    /**
     * @brief Less-than comparison with an external version string.
     * @details Compares major, minor, and patch components numerically.
     * @param ver Version string to compare against.
     * @return @c true if the library version is strictly older than @p ver.
     */
    bool operator<(const std::string& ver);

    /**
     * @brief Less-than-or-equal comparison with an external version string.
     * @param ver Version string to compare against.
     * @return @c true if the library version is at most @p ver.
     */
    bool operator<=(const std::string& ver);

    /**
     * @brief Greater-than comparison with an external version string.
     * @param ver Version string to compare against.
     * @return @c true if the library version is strictly newer than @p ver.
     */
    bool operator>(const std::string& ver);

    /**
     * @brief Greater-than-or-equal comparison with an external version string.
     * @param ver Version string to compare against.
     * @return @c true if the library version is at least @p ver.
     */
    bool operator>=(const std::string& ver);

    /**
     * @brief Equality comparison with an external version string.
     * @param ver Version string to compare against.
     * @return @c true if both short versions are identical.
     */
    bool operator==(const std::string& ver);

    /**
     * @brief Inequality comparison with an external version string.
     * @param ver Version string to compare against.
     * @return @c true if the short versions differ.
     */
    bool operator!=(const std::string& ver);

    /**
     * @brief Static wrapper: test exact equality of the short version.
     * @param ver Expected short version string.
     * @return @c true if equal.
     */
    static bool isEqual(const std::string& ver)     {if(this_ver == NULL) gGIT::this_ver  = new gGIT(); return (*gGIT::this_ver)==ver;};

    /**
     * @brief Static wrapper: test inequality of the short version.
     * @param ver Version string to compare against.
     * @return @c true if not equal.
     */
    static bool isNotEqual(const std::string& ver)  {if(this_ver == NULL) gGIT::this_ver  = new gGIT(); return (*gGIT::this_ver)!=ver;};

    /**
     * @brief Static wrapper: test whether the library is older than @p ver.
     * @param ver Version string to compare against.
     * @return @c true if the library version is strictly less than @p ver.
     */
    static bool isOlderThan(const std::string& ver) {if(this_ver == NULL) gGIT::this_ver  = new gGIT(); return (*gGIT::this_ver)< ver;};

    /**
     * @brief Static wrapper: test whether the library is at least as new as @p ver.
     * @param ver Version string to compare against.
     * @return @c true if the library version is greater than or equal to @p ver.
     */
    static bool isNewerThan(const std::string& ver) {if(this_ver == NULL) gGIT::this_ver  = new gGIT(); return (*gGIT::this_ver)>=ver;};

  };

}

#endif //DSD_VERSION_H
